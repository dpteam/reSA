# HISTORY.md — Work log

> Append-only. Newest entries go **on top**. One entry per work session / logical change.
> For the upstream history, use `git log`. This file records *our* (fork-specific) work and reasoning.

Legend: ✅ verified (built/tested) · ⚠️ unverified (no toolchain) · 🚧 in progress

---

## 2026-10-11 — Standalone foundation + project memory docs ⚠️

**Direction:** start converting the mod into a standalone game (`WinMain` instead of `DllMain`), and set up the
session-memory documents requested in `PROMPT.md`.

**Added**
* `AGENT.md` — working rules (docs to maintain, hard rules, build, git, definition of done).
* `MEMORY.md` — curated project knowledge (architecture, `StaticRef`, standalone blockers, roadmap).
* `HISTORY.md` (this file) — the log.
* `RESUME.md` — context snapshot for the next session.
* `source/Bootstrap.{h,cpp}` — `notsa::bootstrap::LoadConfigurations()`, the config-loading startup logic, now shared
  between the mod and standalone entry points.
* `source/StandaloneStorage.cpp` — address-keyed, zero-initialized, `VirtualAlloc`-backed storage for the game's globals
  in the standalone build.

**Changed**
* `source/Base.h` — `StaticRef<T>(addr)` now uses the new backing storage in the standalone build (the mod and
  `DUMP_HOOKS_ONLY` behaviors are unchanged).
* `source/dllmain.cpp` — `DllMain` is now compiled **only** in the mod build (`#ifndef NOTSA_STANDALONE`); config
  loading moved to `Bootstrap.cpp`; `GetDLLHandle()` returns the exe's module in standalone.
* `source/app/platform/win/WinMain.cpp` — replaced the standalone `WinMain` stub ("meant to be used for dumping hooks
  only") with a real entry point that boots, installs hooks and runs `NOTSA_WinMain()`.
* `source/dllmain.h` — documented `GetDLLHandle()` semantics in both builds.

**Notes / why**
* The reversing is far from complete (~835 `plugin::Call` sites into the original executable), so the standalone exe
  will not run the full game yet. This session builds the *scaffolding* and documents the road ahead in `MEMORY.md`.
* The new storage deliberately avoids `new`/`std` containers because the global `operator new` routes to `CMemoryMgr`
  even in standalone, and `StaticRef` runs during static initialization (before any game init).
* **No compilation was performed** — `cmake`/`conan`/`cl` are not installed in this environment. All changes are
  conservative and guarded so the default (mod) build is unaffected.

**Verification:** none (no toolchain available). Reviewed by hand only.

---

### Follow-up (same day): external references (`refs/`) + standalone build option

* Added a gitignored **`refs/`** folder for read-only third-party reference sources (`.gitignore`: `/refs/*`,
  `!/refs/README.md`) plus `refs/README.md` describing what to drop in (re3 `master`, reVC `miami`, `librw`) and how
  to use it (architecture only; **no code copying** — incompatible licenses).
* `conanfile.py`: added `True` to the `standalone` option so the standalone executable can actually be configured
  (`python setup.py -o standalone=True`); upstream only allowed `False`/`'dump_hooks_only'`.
* `README.md`: corrected the standalone build instructions (the earlier `-o GTASA_STANDALONE=ON` was wrong — that's a
  raw CMake var, not a Conan option).
* `MEMORY.md`/`AGENT.md`: documented `refs/`, and the key finding that **RenderWare is address-called into
  `gta_sa.exe`** (not a linked library) ⇒ the standalone build needs a real RW implementation (likely `librw`).

**Verification:** none (still no toolchain). `conanfile.py`, `.gitignore`, docs only.

---

### Follow-up (same day, 2): cloned the references and mined them for a standalone plan

* Cloned the references into the gitignored `refs/` (via the `rocketguedes/re3` mirror, since `GTAmodding/re3` is
  DMCA'd): `refs/re3` (III, `master`), `refs/reVC` (VC, `miami`), `refs/librw` (RenderWare).
* Read them and extracted concrete answers to our blockers (all recorded in `MEMORY.md` §5b and `refs/README.md`):
  * **re3/reVC are the same lineage as us** — same boot flow and subsystem names (`MemoryMgr`, `MemoryHeap`,
    `TxdStore`, `RwHelper`, `VisibilityPlugins`); their `WinMain` mirrors our `NOTSA_WinMain` almost 1:1.
  * **RenderWare:** they keep the classic `Rw*` C API and add a ~2100-line **compat shim** (`src/fakerw/*`) that maps
    it onto librw's `rw::` C++ API. ⇒ We don't have to rewrite every RW call site, just provide such a shim.
  * **librw is C++-only now** (`rw::`); its C API ended 2019-08-04. re3/reVC pin it at `5501c4fd` (2021-08-19).
  * **Memory:** re3's custom allocator is opt-in (`USE_CUSTOM_ALLOCATOR`); OFF ⇒ plain `malloc`/`free`;
    ON ⇒ `InitMemoryMgr()` is called first thing in `WinMain`.

**Verification:** reference reading only; no code changes to our tree, no build.

---

### Follow-up (same day, 3): toolchain install + two corrections

* Installed **Conan 2.33 / CMake 4.4 / Ninja 1.13** via pip (no admin). `conan profile detect` → msvc **194**.
* Ran `python setup.py -s compiler.version=194`: it downloaded the recipes/toolchain and then **failed at the
  CMake/VS step**. Diagnosis (see `MEMORY.md` §2 "Local dev environment"): the VS 2022 instance is **incomplete** —
  it has the *.NET desktop* workload + `VC.Tools.x86.x64`, but **no "Desktop development with C++" workload and no
  Windows SDK**. Hence (a) no `windows.h` (C1083), (b) CMake's VS generator says *"could not find any instance of
  Visual Studio"* (it only accepts complete instances). **Fix requires admin:** add the *Desktop development with C++*
  workload via the VS Installer (also brings the Windows SDK).
* **Correction to an earlier note:** the global `operator new/delete` is **not** backed by `CMemoryMgr`. The
  `MEMORY_MGR_USE_MEMORY_HEAP` macro (which would enable that) is **never defined**, so that block is dead code;
  the only live global overrides call the game's `0x82119A…` and are `#ifndef NOTSA_STANDALONE`. ⇒ In standalone the
  global `new` is the plain CRT one. The real early blocker is instead `CMemoryMgr::Malloc/Free/…` falling back to
  `plugin::Call(0x72F420…)`. Code comments in `Base.h`/`StandaloneStorage.cpp` corrected accordingly.
* **Correction:** added `MEMORY.md` §2 "Local dev environment" and refined `RESUME.md` with the exact diagnosis.

**Verification:** no build (blocked on the missing workload/SDK). Conan reached the CMake step and failed there.

---

### Follow-up (same day, 4): first successful build + standalone boots to game-data loading 🎉

* **Mod build works:** `bin/Debug/gta_reversed.asi` (~47 MB).
* **Standalone build works:** `bin/Debug/gta_reversed.exe` (~45 MB).
  (Both: `python setup.py --build -s compiler.version=194`; note that wrapper reports exit 1 even on success — trust
  `cmake --build build --config Debug` instead, which returns 0.)

* **Ran the standalone exe** and used `logs/log.log` (the crash handler dumps a full call stack) to iterate:
  1. 🐛 **Fixed a real hook-framework bug:** `RHManager::InstallVirtual` and `RH_ScopedVirtualClass` guarded on
     `NOTSA_STANDALONE_DUMP_HOOKS_ONLY` instead of `NOTSA_STANDALONE`, so the full standalone build still tried to read
     the original game's vtables and threw. Now the standalone build also uses `NullHook` / empty VMTInfo.
     (`RHManager.cpp`, `ReversibleHooks.h`.)
  2. 🐛 **Fixed the memory-manager blocker:** under `NOTSA_STANDALONE`, `CMemoryMgr::Malloc`/`Free`/`Realloc`/`Calloc`/
     `MallocAlign`/`FreeAlign` now use the CRT instead of `plugin::Call` into `gta_sa.exe` (the 2-arg `MallocAlign`
     called `0x72F4C0` unconditionally). (`game_sa/MemoryMgr.cpp`.)
  3. ✅ **It now boots** through `InjectHooksMain()` (425 ms) and into the game, up to
     `CdStreamOpen: MODELS\GTA3.IMG` — i.e. **it now needs the game assets**. Next expected blocker is RenderWare
     init (`RsRwInitialize`), which needs librw.

* Docs: `MEMORY.md` gained §5a (boot progress), §8 (running/debugging + **windowed-mode/RDP** note — windowed mode is
  *not* currently enforced in standalone, needs solving before rendering).

**Verification:** built both configs (exit 0 via CMake); ran the standalone exe and read its crash log. Mod config was
**not** re-built after the shared-file changes (the changes are purely additive `#ifdef NOTSA_STANDALONE` branches, so
the mod path is byte-identical) — mark as not re-verified.
