"""Extract the parameter count ("arity") of every GTA SA script opcode from the original's dispatchers.

Why this exists: the standalone build has no original dispatcher to fall back on, and it cannot skip an
unknown opcode without knowing how many operands to consume - guessing desynchronises the instruction
stream, which is far worse than a crash. The arity is not in any data table, but it IS in the code: nearly
every case begins

    6A <n>              push  n
    8B CE               mov   ecx, esi
    E8 <rel32>          call  0x464080      ; CRunningScript::CollectParameters

so `n` is the number of params the command reads. This walks all 27 dispatchers, resolves every case, and
harvests that `n`.

Output categories, because treating them alike would be the bug:
  * fixed(n)   - a literal `push n` before CollectParameters. Safe to skip n params.
  * none       - the case never calls CollectParameters (it reads nothing, or reads them by other means
                 such as PeekData/CollectResult, or is a label-return style command).
  * irregular  - the case calls CollectParameters with a *computed* count, or more than once, or the body
                 does not match the expected prologue at all. These must NOT be auto-skipped; they are
                 listed so a human/agent can decide per opcode.

Usage:
    python tools/opcode_arity.py            # summary + the irregular list
    python tools/opcode_arity.py --emit     # emit a C++ table to stdout
"""
import re
import struct
import sys

import pefile

EXE = r"refs\gtasa10us\gta_sa.exe"
COLLECT_PARAMS = 0x464080
DISPATCHERS_VA = 0x8A6168
NUM_DISPATCHERS = 27

pe = pefile.PE(EXE, fast_load=True)
BASE = pe.OPTIONAL_HEADER.ImageBase


def file_off(va):
    rva = va - BASE
    for s in pe.sections:
        if s.VirtualAddress <= rva < s.VirtualAddress + max(s.Misc_VirtualSize, s.SizeOfRawData):
            return s.PointerToRawData + (rva - s.VirtualAddress)
    return None


def u8(va, off=0):
    o = file_off(va + off)
    return pe.__data__[o] if o is not None else None


def u32(va, off=0):
    o = file_off(va + off)
    return struct.unpack('<I', pe.__data__[o:o + 4])[0] if o is not None and o + 4 <= len(pe.__data__) else None


def parse_bound_and_jmp(disp):
    """Returns (bound, sel_table_or_None, tgt_table) for a dispatcher.

    Anchored on the ONE unambiguous instruction in the switch header - `jmp DWORD PTR [eax*4+disp32]`
    (FF 24 85 ib) - and read BACKWARDS from it, because scanning forward for a byte equal to 0x05 or 0x8D
    matches immediates belonging to unrelated instructions (measured: it picked up the ScriptParams
    address 0xA43C78 and produced nonsense opcodes). Within the ~24 bytes before the jmp we reliably find:
      * 83 F8 <ib>   cmp eax, imm8      -> bound  (the `ja` default threshold)
      * 0F B6/B7 80 <disp32>  movzx eax, BYTE [eax+disp32]  -> the byte-selector table (shape A only)
    """
    o = file_off(disp)
    if o is None:
        return None
    win = pe.__data__[o:o + 0x120]
    jmp_at = None
    for i in range(len(win) - 6):
        # jmp DWORD PTR [reg*4 + disp32]  ==  FF /4 <SIB> <disp32>, where SIB has scale=10 (4x) and
        # base=101 (disp32, no base reg): (sib & 0xC7) == 0x85. The INDEX register varies - 0x489500 uses
        # eax (0x85) while 0x469390 uses ecx (0x8D) - so match the form, not one encoding.
        if win[i] == 0xFF and win[i + 1] == 0x24 and ((win[i + 2] or 0) & 0xC7) == 0x85:
            jmp_at = i
            break
    if jmp_at is None:
        return None
    tgt = struct.unpack('<I', win[jmp_at + 3:jmp_at + 7])[0]

    bound = sel = None
    for i in range(max(0, jmp_at - 64), jmp_at):
        # cmp <r32>, imm8 is 83 /7 ib with ModRM F8..FF (eax,ecx,edx,ebx,esi,edi). Which register the
        # compiler chose does not matter here - the value is the switch bound. (0x469390 uses ecx, which
        # is why matching only `83 F8` silently dropped three dispatchers and broke the chained coverage.)
        if bound is None and win[i] == 0x83 and 0xF8 <= (win[i + 1] or 0) <= 0xFF:
            bound = win[i + 2]
        elif bound is None and win[i] == 0x3D:
            bound = struct.unpack('<I', win[i + 1:i + 5])[0]
        # movzx <r32>, BYTE PTR [reg + disp32] == 0F B6 <modrm> <disp32>, with mod=10 and rm != 100
        # (no SIB). Index register varies again: eax in 0x489500 (0x80), ecx in 0x469390 (0x89).
        if sel is None and win[i] == 0x0F and win[i + 1] in (0xB6, 0xB7) and \
           ((win[i + 2] or 0) & 0xC0) == 0x80 and (win[i + 2] & 0x07) != 4:
            sel = struct.unpack('<I', win[i + 3:i + 7])[0]
    if bound is None:
        return None
    return bound, sel, tgt


def parse_switch(disp):
    """Returns (base, bound, sel_table_or_None, tgt_table). Handles both dispatcher shapes.

    `base` is DERIVED, not parsed: the 27 dispatchers partition the opcode space into *contiguous* ranges
    (verified - bucket 7's dispatcher covers 703..798 while bucket 6's runs past 699, so the ranges chain
    rather than align to multiples of 100). Chaining the bounds therefore gives every start exactly:
        start[0] = 0 ;  start[b] = start[b-1] + bound[b-1] + 1
    which is immune to the immediate-vs-opcode ambiguity that broke byte-scanning for `add eax, imm32`.
    Callers must therefore pass the base in (see main()).
    """
    r = parse_bound_and_jmp(disp)
    if r is None:
        return None
    bound, sel, tgt = r
    return bound, sel, tgt


def follow_thunk(va, depth=0):
    """If `va` is a lone `jmp rel32` into a far section, return the real body address (see MEMORY.md)."""
    if depth > 4:
        return va
    o = file_off(va)
    if o is None:
        return va
    if pe.__data__[o] == 0xE9:
        rel = struct.unpack('<i', pe.__data__[o + 1:o + 5])[0]
        return follow_thunk(va + 5 + rel, depth + 1)
    return va


def classify_case(target, end):
    """Inspect one case body and return (kind, arity) with kind in {fixed, none, irregular}.

    `end` is the start of the next case (cases are laid out contiguously), which is what makes the scan
    trustworthy: without it, a 160-byte lookahead sees the *neighbouring* cases' CollectParameters calls
    and every case looks "irregular".
    """
    target = follow_thunk(target)
    if end is None or end <= target:
        end = target + 64
    o = file_off(target)
    if o is None:
        return ('irregular', None)
    body = pe.__data__[o:o + max(0, min(end - target, 240))]
    if len(body) < 9:
        return ('irregular', None)

    # The common prologue, exactly at offset 0:  6A <n> | 8B CE | E8 <rel32 -> CollectParameters>
    if body[0] == 0x6A and body[2:4] == b'\x8b\xce' and body[4] == 0xE8:
        if target + 9 + struct.unpack('<i', body[5:9])[0] == COLLECT_PARAMS:
            n = body[1]
            # ScriptParams is 32 deep, so a larger literal is a misread rather than a fat command.
            if n <= 32:
                return ('fixed', n)
            return ('irregular', None)

    # Otherwise: does the case call CollectParameters at all, and if so how is the count pushed?
    calls = []
    for j in range(0, len(body) - 9):
        if body[j] == 0xE8 and target + j + 5 + struct.unpack('<i', body[j + 1:j + 5])[0] == COLLECT_PARAMS:
            calls.append(j)
    if not calls:
        return ('none', 0)                 # reads no operands at entry -> skipping is just "continue"
    if len(calls) == 1:
        j = calls[0]
        if j >= 3 and body[j - 3] == 0x6A and body[j - 1:j + 1] == b'\x8b\xce' and body[j - 2] <= 32:
            return ('fixed', body[j - 2])  # same shape, just not at offset 0
    return ('irregular', None)             # computed count, or several collections -> decide by hand


def parse_actual_base(disp):
    """Read the base the code itself uses, for CROSS-CHECKING the chained derivation.

    The switch normalises the opcode with either `add eax, imm32` (05 ib..) or `lea r, [r - imm]`
    (8D .. disp32), and that instruction sits right before the `cmp`/`ja`. Searching for a bare 0x05 byte
    is unreliable (it matches immediates of unrelated instructions), so this only ever *verifies* a
    candidate; it is never the source of truth.
    """
    o = file_off(disp)
    if o is None:
        return None
    win = pe.__data__[o:o + 0x120]
    # Find the `cmp <r>,imm8` that gives the bound, then look back a few bytes for the normaliser. Forms
    # seen in this binary (all measured): 05 imm32 (add eax,imm32), 83 C0 ib (add eax,imm8), and
    # 8D <modrm> disp32 with mod=10 and rm!=100 (lea r,[r+disp32]) - e.g. 0x478000 does `lea eax,[edi-0x960]`.
    for i in range(len(win) - 6):
        if win[i] == 0x83 and 0xF8 <= win[i + 1] <= 0xFF:
            for j in range(max(0, i - 14), i):
                if win[j] == 0x05:                                  # add eax, imm32
                    return struct.unpack('<i', win[j + 1:j + 5])[0]
                if win[j] == 0x83 and (win[j + 1] & 0xC7) == 0xC0:   # add eax, imm8
                    return struct.unpack('<b', win[j + 2:j + 3])[0]
                if win[j] == 0x8D and (win[j + 1] & 0xC0) == 0x80 and (win[j + 1] & 0x07) != 4:
                    return struct.unpack('<i', win[j + 2:j + 6])[0]  # lea r,[r+disp32]
            # Do NOT give up here: an earlier `cmp` may have been the SEH/stack probe rather than the
            # switch bound, so keep scanning forward for the next `cmp <r>,imm8`. (Returning None on the
            # first miss is what dropped 11 dispatchers that the previous, narrower version handled.)
    # No normaliser anywhere: the switch compares the opcode itself (measured on bucket 0, 0x465E60 -
    # `mov eax,[esp+0x10]` then `cmp eax,0x63` with nothing between), which means base == 0. Confirm that
    # reading before concluding it, since a missed normaliser would silently shift a whole bucket.
    for i in range(len(win) - 6):
        if win[i] == 0x83 and 0xF8 <= win[i + 1] <= 0xFF:
            for j in range(max(0, i - 22), i):
                if win[j] == 0x8B and (win[j + 1] & 0xC7) == 0x44:   # mov r32, [esp+ib]  (the arg load)
                    return 0
            break
    return None


def main():
    table = {}
    irregular = []
    unparsed = []

    # Pass 1: parse every dispatcher. The base comes from the dispatcher's OWN code (`add eax, imm32` /
    # `lea r,[r-imm]`); the chained derivation is kept only as a cross-check, because the opcode space has
    # real GAPS - measuring it showed 12 of the 27 dispatchers disagree with the chain by 17..111 opcodes.
    # A wrong base misassigns arity for a whole bucket, and a wrong arity desynchronises the instruction
    # stream, so the code's statement is authoritative.
    parsed = []
    nobase = []
    start = 0
    for b in range(NUM_DISPATCHERS):
        disp = u32(DISPATCHERS_VA, 4 * b)
        r = parse_switch(disp)
        if r is None:
            unparsed.append((b, disp))
            parsed.append(None)
            start += 100                      # keep the chain estimate moving so later buckets show up
            continue
        bound, sel, tgt = r
        base = parse_actual_base(disp)
        if base is None:
            nobase.append((b, disp, bound))
        parsed.append((disp, base, bound, sel, tgt))
        start += bound + 1
    print('dispatchers: %d parsed, %d unparsed, %d without a readable base' %
          (NUM_DISPATCHERS - len(unparsed), len(unparsed), len(nobase)))
    if nobase:
        for b, disp, bound in nobase:
            print('   bucket %2d %#x (bound %d): base not readable - its opcodes are LEFT UNCLASSIFIED'
                  % (b, disp, bound))
    print()

    # Pass 2: classify, using each dispatcher's own sorted case addresses as body boundaries. Dispatchers
    # whose base could not be read are SKIPPED rather than guessed at.
    for b, ent in enumerate(parsed):
        if ent is None or ent[1] is None:
            continue
        disp, base, bound, sel, tgt = ent
        cases = []
        for idx in range(bound + 1):
            s = u8(sel, idx) if sel else idx
            t = follow_thunk(u32(tgt, 4 * s))
            cases.append((idx - base, t))
        by_addr = sorted({t for _o, t in cases})
        nxt = {}
        for i, a in enumerate(by_addr):
            nxt[a] = by_addr[i + 1] if i + 1 < len(by_addr) else None
        for op, t in cases:
            kind, n = classify_case(t, nxt.get(t))
            table[op] = (kind, n, t)
            if kind == 'irregular':
                irregular.append((op, t))

    fixed = sum(1 for k, _n, _t in table.values() if k == 'fixed')
    none_ = sum(1 for k, _n, _t in table.values() if k == 'none')
    irr = len(irregular)
    print('dispatchers parsed: %d/%d (unparsed: %s)' %
          (NUM_DISPATCHERS - len(unparsed), NUM_DISPATCHERS,
           ', '.join('%#x' % d for _b, d in unparsed) or 'none'))
    print('opcodes classified : %d  (fixed=%d  reads-nothing=%d  irregular=%d)'
          % (len(table), fixed, none_, irr))
    print()
    dist = {}
    for k, n, _t in table.values():
        if k == 'fixed':
            dist[n] = dist.get(n, 0) + 1
    print('arity histogram (fixed cases): ' + ' '.join('%d:%d' % kv for kv in sorted(dist.items())))
    print()
    if irregular:
        print('IRREGULAR - must be decided individually, never auto-skipped (first 60):')
        for op, t in sorted(irregular)[:60]:
            print('   %#05x (%d)  case=%#010x' % (op, op, t))
        if len(irregular) > 60:
            print('   ... and %d more' % (len(irregular) - 60))

    if '--emit' in sys.argv:
        print()
        print('// Generated by tools/opcode_arity.py from refs/gtasa10us/gta_sa.exe.')
        print('// value = param count, or -1 = reads nothing, or -2 = irregular (do not auto-skip).')
        print('static constexpr int16 OPCODE_ARITY[] = {')
        row = []
        for op in range(max(table) + 1):
            k, n, _t = table.get(op, ('irregular', None, 0))
            v = -2 if k == 'irregular' else (n if k == 'fixed' else -1)
            row.append(str(v))
            if len(row) == 20:
                print('    ' + ', '.join(row) + ',')
                row = []
        if row:
            print('    ' + ', '.join(row) + ',')
        print('};')


if __name__ == '__main__':
    main()
