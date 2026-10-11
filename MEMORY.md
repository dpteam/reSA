# MEMORY.md — Living project knowledge

> **What this is:** the project's long-term memory. Unlike `HISTORY.md` (chronological log) this file is
> *rewritten/curated*: it should always describe the **current** state, verified facts, and open problems.
> Append new findings; move stale ones to `HISTORY.md` if they ever matter again.
>
> **Confidence tags:** ✅ verified by reading code/config · ⚠️ believed but unverified · ❓ open question.

---

## 1. Project identity

* This working copy is the fork **`dpteam/reSA`** (`origin`), tracking **`gta-reversed/gta-reversed`** (`upstream`). ✅
* Base: the `gta-reversed` project — a from-scratch C++ rewrite/reversal of GTA: San Andreas. ✅
* Active branch: `master`. ✅
* **Mission of this fork:** evolve the *mod* (ASI) into a **standalone game** (own `.exe`, `WinMain`, no original
  `gta_sa.exe`). The mod build must keep working. ✅

## 2. Toolchain & build (✅ from `README.md`, `CMakeLists.txt`, `source/CMakeLists.txt`)

* Language/std: **C++23**; compiler: **MSVC**; target: **Win32 (x86)**; platform: Windows.
* Build system: **CMake ≥ 4.2** (`CMakePresets.json`, `setup.py` wraps Conan + CMake); dependency manager: **Conan 2.x**.
* Recommended: **Visual Studio 2026** + VS's native CMake integration.
* Compile options include `/MP /Zc:preprocessor /W3` and **`COMPILE_WARNING_AS_ERROR ON`** — warnings are errors! ✅
  New code must be warning-clean.
* PCH: `target_precompile_headers(... StdInc.cpp)` → **every `.cpp` must `#include "StdInc.h"` first**. ✅ (watch out
  for new files.)

### Build modes ✅
| CMake option | Target | Entry point | Purpose |
|---|---|---|---|
| *(default)* | `gta_reversed.asi` (MODULE) | `DllMain` (`source/dllmain.cpp`) | Injected into the original `gta_sa.exe`; real hooks redirect game functions to our reversed code. |
| `GTASA_STANDALONE=ON` | `gta_reversed.exe` (WIN32) | `WinMain` (`source/app/platform/win/WinMain.cpp`) | The standalone game. **WIP.** |
| `..._STANDALONE_DUMP_HOOKS_ONLY=ON` | exe | `WinMain` | CI-only: dumps hook metadata as JSON, does nothing else. |

Both suffix/output dir: `bin/<config>/`. ✅

Conan wrapper: `conanfile.py` option **`standalone`** = `False` (default) | `True` | `'dump_hooks_only'`; the Conan
toolchain maps it onto `GTASA_STANDALONE` / `GTASA_STANDALONE_DUMP_HOOKS_ONLY`. `True` was **added by us** (upstream
only had `False`/`'dump_hooks_only'`), so standalone can be configured via `python setup.py -o standalone=True`. ⚠️
CI (`.github/workflows/update-re-progress.yml`) uses `standalone: dump_hooks_only`. ✅

## 3. Architecture essentials

### Entry points & startup
* **Mod:** `DllMain(DLL_PROCESS_ATTACH)` → `DisplayConsole`, `LoadSymbols`, `Logging::CreateInstance`, `CommandLine::Load`,
  `LoadConfigurations()`, `RHManager::CreateInstance()`, `InjectHooksMain()`. On detach: reverse. ✅
* **Standalone:** `WinMain` → `BootstrapStandalone()` (console, command line, symbols, logging) → `RHManager::CreateInstance()`
  → `InjectHooksMain()` → `NOTSA_WinMain(...)` (the reversed game main). ✅
* Shared startup logic lives in `source/Bootstrap.{h,cpp}` (`notsa::bootstrap::LoadConfigurations()`). ✅
* `NOTSA_WinMain` (`source/app/platform/win/WinMain.cpp`) is the reversed game main (GTA `0x748710`); in the mod build it is
  *hooked in* via `InjectWinMainStuff()`. In standalone we call it directly. ✅

### Hooking (`source/reversiblehooks/`)
* `InjectHooksMain()` (`source/InjectHooksMain.cpp`) is the single place that registers every reversed function, using
  `RH_Scoped*` / `RH_Install*` macros. New reversed classes **must** be added there (see `docs/HookRegistration.md`). ✅
* `RHManager::InstallStatic` instantiates a **real** `StaticTwoWayHook` in the mod build, but a **`NullHook`** under
  `NOTSA_STANDALONE**. A `NullHook` does nothing — so in the standalone build, "hooking" is documentation only and
  reversed functions are simply called directly. ✅
* Script-command hooks still work in standalone (`ScriptCommandHook` is kept). ✅

### The global-variable problem — `StaticRef` (`source/Base.h`) ⭐
* The game's globals are accessed via `StaticRef<T>(addr)`, e.g. `static inline auto& currArea = StaticRef<int32>(0xB72914);`. ✅
* `StaticRef` behavior by build:
  * **Mod:** `*reinterpret_cast<T*>(addr)` — the global really lives there. ✅
  * **DUMP_HOOKS_ONLY:** a single `static` per-type buffer (⚠️ known aliasing bug, fine for dumping). ✅
  * **Standalone (new):** address-keyed backing storage in our own memory via
    `notsa::detail::GetStandaloneStaticStorage()` (`source/StandaloneStorage.cpp`). ✅
    * Trade-off: every `StaticRef` call in standalone now takes a mutex and does a hash lookup. Fine as a first
      step, but worth revisiting (eg. compile-time address keys) if it shows up in profiles. ⚠️
* Scale: **~2373 `StaticRef<...>` uses**. ⚠️ (Measured via grep; re-measure with
  `Get-ChildItem -Recurse -Include *.cpp,*.h,*.hpp -Path source | Select-String -Pattern 'StaticRef<'`.)

### RenderWare is address-called, not linked ⚠️ (important for standalone)
* The `source/game_sa/RenderWare/rw/**` files are largely **headers + thunks that call absolute addresses in
  `gta_sa.exe`** — eg. `rwcore.cpp`: `void* RxHeapAlloc(...) { return ((void*(__cdecl*)(...))0x809AA0)(...); }`.
  So "RenderWare" is *not* a linked library; it's part of the original executable. ✅
* ⇒ In the standalone build there is **no RenderWare at all**. We must supply one ourselves. The natural candidate
  is **`librw`** (`aap/librw`), the RenderWare reimplementation used by re3/reVC. (Ref only — see `refs/`.) ⚠️
* Also note `target_link_libraries` only links `ddraw/Winmm/dxguid/strmiids/d3d9` — no RW lib exists. ✅

## 4. Scale of the remaining work (⚠️ measured, but re-verify)

* **~835 `plugin::Call*` uses across ~190 files.** These are **absolute-address calls into the original executable**.
  They cannot work in the standalone build (the original code isn't mapped there). This is *the* long-tail blocker. ⚠️
* Progress is tracked in `docs/ReversedClasses.md` and `docs/hooks.json`; upstream estimates ~50–60% reversed. ⚠️

## 5. Known blockers / gotchas for the standalone build 🚧

1. **`plugin::Call` / `CallAndReturn` to absolute addresses.** Non-reversed code physically lives in `gta_sa.exe`.
   Standalone needs either (a) all of it reversed, or (b) a strategy to satisfy those calls. ~835 sites. ⚠️
2. **Global `operator new` overloads.** `source/game_sa/MemoryMgr.h` defines `MEMORY_MGR_OVERLOAD_NEW_DELETE`
   **unconditionally**, and `MemoryMgr.cpp` then defines `operator new(uint32)`/`operator delete` to use `CMemoryMgr`.
   On x86 `uint32 == size_t`, so this is the active global `new` **even in standalone**. ⇒ **Any heap allocation
   (including `std::` containers, and even `notsa::Singleton::CreateInstance()`, which does `new T{}`) before
   `CMemoryMgr::Init()` is dangerous.** This is why `StandaloneStorage.cpp` allocates with `VirtualAlloc`, not `new`. ⚠️
   (`MemoryMgr.cpp` *does* correctly guard the *other* `operator new(size_t)` — the one that calls `0x82119A` — with
   `#ifndef NOTSA_STANDALONE`. ✅)
3. **Startup that isn't reversed** (RenderWare init, D3D9, file system, audio, save system) will fail until reversed. ⚠️
   * See §3 "RenderWare is address-called" — RW itself has to be *provided* (likely via `librw`), it can't be reversed
     because GTA:SA's RW was not written by this project.
4. **`g_ConfigurationMgr` has internal linkage** (`static inline class {...} g_ConfigurationMgr;` in
   `extensions/Configuration.hpp`) ⇒ **one copy per translation unit**. Therefore only **one** TU may ever load/read
   the INI; today that TU is `Bootstrap.cpp`. Do **not** call `g_ConfigurationMgr` from a second TU. ⚠️
5. **`GetDLLHandle()`** (`dllmain.h`) must resolve to a real module in standalone; it now returns
   `GetModuleHandleA(nullptr)` (the exe). Used by `VMTInfo.cpp` (`GetProcAddress`) to find exported vtables — this
   in turn relies on `NOTSA_EXPORT_VTABLE` (`__declspec(dllexport)`), which works from an exe too. ⚠️

## 6. Proposed standalone roadmap ❓

* **Phase 0 — entry point & storage (started):** real `WinMain`, shared bootstrap, config loading, and address-keyed
  storage for globals. *Done in this session (unverified build).*
* **Phase 1 — self-contained memory:** make `CMemoryMgr` initialize/behave in standalone; decouple global `new/delete`
  from the game allocator (or guarantee init order); audit static-initialization heap usage.
* **Phase 2 — eliminate `plugin::Call`:** reverse the remaining functions reachable from startup (or add explicit,
  clearly-marked stubs that fail loudly). Drive this from the actual startup call graph, not alphabetically.
* **Phase 3 — subsystems:** RenderWare (likely `librw`) / D3D9, windowing/input, filesystem, audio, save system.
  Note: RW/D3D9 in SA is engine code we can't "reverse" — it must be *replaced*, see `refs/` + `MEMORY.md` §5.3.
* **Phase 4 — packaging:** exe name/assets/install flow, replace the ASI install flow in `contrib/install.py`.

Also cross-check all of this against the closest existing prior art, `refs/re3` (III) and `refs/reVC` (VC) —
complete standalone games built on the same engine family, plus `refs/librw` for RenderWare. ⚠️

## 7. Useful facts / commands

* Hook a global function (mod): `RH_ScopedGlobalInstall(Fn, 0xADDR)`; reversed classes: `RH_ScopedInstall`, etc.
* Hook IDs that matter here: `NOTSA_WinMain @ 0x748710`, `MessageLoop @ 0x746870`, both `.Locked = true`. ✅
* `docs/ai/TipsAndTricks.md` — aggregated reversing knowledge (enums, struct offsets, IDA quirks). ✅
* Grep helper for outstanding work:
  `Select-String -Path source -Pattern 'plugin::Call' -Recurse` .
