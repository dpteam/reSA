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
