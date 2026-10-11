# RESUME.md — Session snapshot

> **How to use:** at the *start* of a session, read this (plus `MEMORY.md`) to restore context.
> At the *end* of a session, rewrite this file: state, in-flight work, next steps, and anything unverified.
> Keep it short — details belong in `MEMORY.md` / `HISTORY.md`.

---

## Last updated
2026-10-11

## State of the tree
* Branch: `master` (fork `dpteam/reSA` = `origin`, upstream `gta-reversed/gta-reversed`).
* Working tree: see `HISTORY.md` (latest entry).
* ✅ **Both configs build** (VS 2022 / MSVC 194, Windows SDK 10.0.26100):
  * Mod: `bin/Debug/gta_reversed.asi`
  * Standalone: `bin/Debug/gta_reversed.exe`
  * Build cmd: `python setup.py --build -s compiler.version=194` (⚠️ wrapper reports exit 1 even on success — trust
    `cmake --build build --config Debug`, which returns 0).
* The `build/` dir is currently configured for **standalone** (`GTASA_STANDALONE=ON`).

## Standalone status (2026-10-11) — it BOOTS, needs assets
`gta_reversed.exe` runs through hook installation and into the game, stopping at `CdStreamOpen: MODELS\GTA3.IMG`
→ **it now needs the GTA:SA game files** next to the exe. Next expected blocker after that: RenderWare init.
See `MEMORY.md` §5a (crash-by-crash progress) and §8 (how to run/debug + **windowed-mode/RDP** caveat: windowed mode
is NOT currently enforced in standalone).

⚠️ **Not re-verified:** the mod build wasn't rebuilt after the shared-file changes (`RHManager.cpp`,
`ReversibleHooks.h`, `MemoryMgr.cpp`). The changes are additive `#ifdef NOTSA_STANDALONE` branches, so the mod path is
unchanged — but a full mod rebuild is still pending.

## Environment (⚠️ verified 2026-10-11 on this machine — may differ elsewhere)
* ✅ **Toolchain is fully working now.** VS 2022 Community has the *Desktop development with C++* workload + Windows
  SDK 10.0.26100 (was incomplete before); `cl` compiles `windows.h`/`d3d9.h`/`dinput.h`. Conan 2.33 / CMake 4.4 /
  Ninja 1.13 installed via pip.
* ✅ **The mod build succeeds**: `python setup.py --build -s compiler.version=194` → `bin/Debug/gta_reversed.asi`.
  ⚠️ That wrapper reports exit code 1 even on success; for a trustworthy code use
  `cmake --build build --config Debug` (returns 0).
* ❗ Profile asks `compiler.version=195` (VS 2026), this machine is **194** ⇒ always pass `-s compiler.version=194`. ⚠️
* Deps build from source (`GTASA_USE_SDL3=ON` default) — slow the first time, then cached in `~/.conan2`. ⚠️

## How to build (once the Windows SDK is present)
```powershell
conan profile detect --force          # only the first time
python setup.py                       # configure + fetch deps (mod/ASI config, x86 Debug)
python setup.py --build               # build  ->  bin/Debug/gta_reversed.asi
# standalone:
python setup.py -o standalone=True --build     # (add -s compiler.version=194 on VS 2022)
```
Verify the toolchain first: `conan --version`, `cmake --version`, and a `cl` compile of a file including `windows.h`.

## What was just done (short)
Started the **mod → standalone game** conversion (scaffolding only):
1. Real standalone `WinMain` that boots and calls `NOTSA_WinMain` (`source/app/platform/win/WinMain.cpp`).
2. `DllMain` compiled only in the mod build (`source/dllmain.cpp`).
3. Shared config startup in `source/Bootstrap.{h,cpp}`.
4. Address-keyed global storage for standalone via `VirtualAlloc` (`source/StandaloneStorage.cpp`, used by
   `StaticRef` in `source/Base.h`).
5. Project-memory docs: `AGENT.md`, `MEMORY.md`, `HISTORY.md`, `RESUME.md`, and the standalone build option
   (`conanfile.py`: `standalone=True`).
6. Gitignored **`refs/`** for read-only third-party references (`refs/README.md`: re3, reVC, librw).

## Reference material (refs/)
`refs/` is gitignored and **may be empty on your machine** — that's fine, it's read-only reference material.
**It is populated here**: `refs/re3` (III), `refs/reVC` (miami), `refs/librw`. See `refs/README.md` for how they were
obtained, and `MEMORY.md` §3 + §5b for what we learned from them. Headline findings:

* re3/reVC are the **same lineage** as us — use them as the boot-sequence/architecture template.
* RenderWare: re3 keeps the classic C API and maps it onto **librw's `rw::` C++ API with a ~2100-line compat shim**
  (`refs/re3/src/fakerw/*`). We should copy *that idea* (not the code).
* librw has no C API anymore; re3/reVC pin it at commit `5501c4fd` (2021-08-19).

## Unverified changes ⚠️
**Everything code-related in the 2026-10-11 entry.** Highest-risk items to compile first:

| File | Risk | What to check |
|---|---|---|
| `source/StandaloneStorage.cpp` | new file, compiled in *all* builds (body guarded) | PCH include, `VirtualAlloc`/hash table, `VERIFY`, no unused vars/warnings |
| `source/Base.h` | affects every TU | `StaticRef` branches; `<cstddef>`; declaration under `#ifdef NOTSA_STANDALONE` |
| `source/Bootstrap.{h,cpp}` | compiled in *all* builds | `g_ConfigurationMgr` usage confined to this TU; PCH include |
| `source/dllmain.cpp` | entry point of the mod build | `#ifndef NOTSA_STANDALONE` guard; `GetDLLHandle` standalone branch |
| `source/app/platform/win/WinMain.cpp` | standalone only | includes (`<clocale>`, `<Bootstrap.h>`), `ScopeGuard` symbols |

## Immediate next steps
1. **Build the mod config first** (`python setup.py`, `python setup.py --build`) to confirm nothing regressed.
2. **Build the standalone config** (`python setup.py -o standalone=True --build`) and fix compile errors.
3. **Try running the standalone exe.** Expect an early crash; capture *where* (console log / debugger). That crash is
   the map for Phase 2 of the roadmap (`MEMORY.md` §6).
4. **Phase 1 (memory):** confirm/adjust `CMemoryMgr` + global `new/delete` behavior for standalone
   (`MEMORY.md` §5.2). This likely blocks everything else.
5. **Read the references** (`refs/README.md`): startup order/bootstrapping (re3), memory manager, and especially how
   RenderWare is provided (librw).

## Open questions ❓
* Is there an agreed design for satisfying the ~835 `plugin::Call` sites in standalone (reversal vs. loader vs. stubs)?
* Should the standalone exe eventually be named `gta_sa.exe` (to reuse the install flow) or stay `gta_reversed.exe`?
* Should `GTASA_STANDALONE` get a dedicated `CMakePresets.json` preset + a CI job so this path stops rotting?
* **RenderWare:** adopt `librw` wholesale, or choose another path? (It's the only realistic way to have RW, since
  SA's RW is closed-source and lives in the original exe.)

## Commands cheat-sheet
```powershell
# configure + build (Windows)
python setup.py
python setup.py --build

# standalone configure (example)
python setup.py -o standalone=True -s build_type=Debug

# find remaining non-reversed code
Get-ChildItem -Recurse -Include *.cpp,*.h,*.hpp -Path source | Select-String -Pattern 'plugin::Call'
```

## refs/ (read-only references, gitignored)
If the folder is present, look at `refs/re3`, `refs/reVC` (miami), `refs/librw` — architecure/boot/RenderWare
reference only, **no code copying** (licenses). See `refs/README.md`.
