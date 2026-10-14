"""Resolve which code handles a given GTA SA script opcode, via the original's dispatcher jump tables.

Header parsing lives in ONE place - tools/opcode_arity.py - and is imported here. Two independent
hand-rolled x86 scanners drifted apart in practice (the duplicate kept matching immediates of unrelated
instructions, turning the ScriptParams address 0xA43C78 into a "base" of +10763384), so this file is now
only a CLI over the shared, verified parser.

Usage:
    python tools/opcode_case.py <dispatcher_va> <opcode> [more opcodes...]
    python tools/opcode_case.py 0x47FA30 0x30D
    python tools/opcode_case.py --table 0x489500        # every case of one bucket
    python tools/opcode_case.py --opcodes               # opcode -> dispatcher + arity, whole space
    python tools/opcode_case.py --dispatchers           # the 27-entry dispatcher table

Then disassemble a case target, e.g.
    objdump -d -M intel --start-address=0x<target> --stop-address=<+0x80> refs/gtasa10us/gta_sa.exe

Notes
* Addresses are for refs/gtasa10us/gta_sa.exe (1.0 US); this project targets a different exe, so match by
  function identity, not address. If a target's first instruction is a lone `jmp` into a far section,
  FOLLOW IT (see MEMORY.md "jump thunks") - `--opcodes`/`--table` already resolve thunks for you.
* The opcode space is NOT partitioned on multiples of 100 (measured: bucket 7 starts at 703, bucket 24 at
  2400), which is why the base is read from the code rather than derived from `opcode / 100`.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import opcode_arity as oa   # the single parser


def build():
    """Returns (table, per-bucket info), where table[opcode] = (kind, arity, case_va)."""
    table = {}
    buckets = []
    for b in range(oa.NUM_DISPATCHERS):
        disp = oa.u32(oa.DISPATCHERS_VA, 4 * b)
        parsed = oa.parse_switch(disp)
        if parsed is None:
            buckets.append((b, disp, None, None, None))
            continue
        bound, sel, tgt = parsed
        base = oa.parse_actual_base(disp)
        buckets.append((b, disp, base, bound, tgt))
        if base is None:
            continue
        cases = []
        for idx in range(bound + 1):
            s = oa.u8(sel, idx) if sel else idx
            cases.append((idx - base, oa.follow_thunk(oa.u32(tgt, 4 * s))))
        addrs = sorted({t for _o, t in cases})
        nxt = {a: (addrs[i + 1] if i + 1 < len(addrs) else None) for i, a in enumerate(addrs)}
        for op, t in cases:
            table[op] = oa.classify_case(t, nxt.get(t)) + (t,)
    return table, buckets


def main():
    if len(sys.argv) < 2:
        print(__doc__.strip())
        return
    arg = sys.argv[1]

    if arg == '--dispatchers':
        for i in range(oa.NUM_DISPATCHERS):
            print('   [%2d] -> %#010x' % (i, oa.u32(oa.DISPATCHERS_VA, 4 * i)))
        return

    if arg == '--opcodes':
        table, _ = build()
        for op in sorted(table):
            kind, n, t = table[op]
            print('   %#05x (%-5d) %-10s %s  case=%#010x' %
                  (op, op, kind, ('%d param(s)' % n if kind == 'fixed' else ''), t))
        return

    if arg == '--table':
        disp = int(sys.argv[2], 16)
        bound, sel, tgt = oa.parse_switch(disp)
        base = oa.parse_actual_base(disp)
        print('dispatcher %#x: base %+d bound %d sel=%s targets=%#x' %
              (disp, base, bound, hex(sel) if sel else 'none (direct)', tgt))
        for idx in range(bound + 1):
            s = oa.u8(sel, idx) if sel else idx
            t = oa.follow_thunk(oa.u32(tgt, 4 * s))
            print('   cmd %4d  sel=%-3d target=%#010x' % (idx - base, s, t))
        return

    disp = int(arg, 16)
    bound, sel, tgt = oa.parse_switch(disp)
    base = oa.parse_actual_base(disp)
    if base is None:
        raise SystemExit('base not readable for %#x - refusing to guess' % disp)
    table, _ = build()
    for a in sys.argv[2:]:
        op = int(a, 0)
        idx = op + base
        print('opcode %#05x (%d)  dispatcher %#x: base %+d bound %d -> index %d' %
              (op, op, disp, base, bound, idx))
        if not (0 <= idx <= bound):
            print('   out of range -> the default (unknown-command) path')
            continue
        s = oa.u8(sel, idx) if sel else idx
        t = oa.follow_thunk(oa.u32(tgt, 4 * s))
        print('   selector = %d, case target = %#010x' % (s, t))
        if op in table:
            kind, n, _arity_case = table[op]
            print('   arity: %s %s' % (kind, ('%d param(s)' % n) if kind == 'fixed' else ''))


if __name__ == '__main__':
    main()
