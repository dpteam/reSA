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

### Local dev environment (this machine, verified 2026-10-11) ⚠️ machine-specific
* **Visual Studio 2022 Community 17.14** (`C:\Program Files\Microsoft Visual Studio\2022\Community`) + BuildTools —
  installed. MSVC toolset **14.44** (`_MSC_VER` 1944 ⇒ Conan `compiler.version=194`), x86 target present,
  `vcvarsall.bat` works. ✅
* ❗ **Windows SDK is NOT installed** — there is no `windows.h` anywhere. `cl /c` on `#include <windows.h>` fails with
  `C1083`. This is the **hard blocker** for any build; fixing it needs admin (VS Installer → "Windows 11 SDK").
  Install command (admin):
  `setup.exe modify --installPath "...\2022\Community" --add Microsoft.VisualStudio.Component.Windows11SDK.22621 --quiet`.
* **Conan / CMake / Ninja** were not installed; being installed via `pip` (no admin needed). CMake can also come from
  Conan's `tool_requires` (profile pins `cmake/4.2.0`), so a separate CMake isn't strictly required. ⚠️
* ❗ **Profile mismatch:** repo profile `conan/profiles/windows-msvc.txt` asks `compiler.version=195` (**VS 2026**),
  this machine has VS 2022 (**194**). Prebuilt binary deps (SDL3/ogg/vorbis/libjpeg-turbo/tracy) are ABI-sensitive ⇒
  build with `python setup.py -s compiler.version=194` (or install VS 2026). ⚠️
* Not admin (so no VS Installer / SDK from the agent). Disk: C: ~54 GB free, E: ~61 GB free. ✅

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
* **How the references solve it (verified):** re3/reVC do **not** rewrite their `Rw*` call sites. They keep the classic
  C API and add a thin **compat shim** that maps it onto librw's `rw::` C++ API:
  `refs/re3/src/fakerw/{rwcore,rwplcore,rpworld,rpskin,rpmatfx,rphanim,rpanisot,rtquat,rtbmp,rtpng,rtcharse}.h`
  + `fake.cpp` (~2100 lines total). Eg. `typedef rw::Raster RwRaster;`, `RwCameraClear()` → `rw::Camera::clear()`.
  ⇒ For us that means: add librw, write such a shim, and the address-thunks in
  `source/game_sa/RenderWare/rw/*.cpp` get real implementations. **No mass rewrite of call sites needed.** ✅
* **librw version matters:** `aap/librw` has only `master`, which is already the `rw::` namespaced C++ API (it stopped
  being the C API in commit `14c773d`, 2019-08-04). re3/reVC pin librw at **`5501c4fd…` (2021-08-19)** — the same
  C++ API. So librw + a shim is the path; do **not** expect a drop-in `RwCameraClear`. ✅

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

## 5b. What the references tell us (✅ verified by reading `refs/re3` & `refs/reVC`)

Our project and re3/reVC are the **same lineage** — same boot flow (`RsEventHandler(rsINITIALIZE/…)`,
`CGame::Initialise`, `InitialiseOnceBeforeRW`), same subsystems (`MemoryMgr`, `MemoryHeap`, `TxdStore`, `RwHelper`,
`VisibilityPlugins`). So re3/reVC are a near-perfect template. Concrete answers they give:

| Our blocker | What re3/reVC do |
|---|---|
| RenderWare | Bind **librw** (submodule, pinned `5501c4fd`) + a ~2100-line **compat shim** (`src/fakerw/*`). See §3. |
| Global `operator new`/`CMemoryMgr` | The custom allocator is **opt-in** (`USE_CUSTOM_ALLOCATOR`). When OFF, `MemoryMgrMalloc`/`Free`/… just call `malloc`/`free`. When ON, `WinMain` calls `InitMemoryMgr()` as the **first** thing (`gMainHeap.Init(128MB)`). ⇒ Either use the CRT allocator, or guarantee init order. ✅ |
| Startup order | `refs/re3/src/skel/win/win.cpp` `WinMain` (~line 2012) mirrors our `NOTSA_WinMain` almost 1:1 — window class, `RsEventHandler(rsRWINITIALIZE)`, command line, etc. Good checklist. ✅ |
| RW memory functions | librw is handed `RwMemoryFunctions memFuncs = { Malloc, Free, Realloc, Calloc }` via `RwEngineInit(memFuncs, …)`. ✅ |
| Platform/window | `refs/re3/src/skel/{win,glfw}/`; we have `source/oswrapper/` + `app/platform/win/`. GLFW is an alternative to raw Win32 if we ever want it. ✅ |

**Caveat:** III/VC ≠ SA. Different class layouts, addresses and logic — borrow *architecture and boot order*, never code
(and never copy: incompatible licenses, see `refs/README.md`). ✅

### 5c. `WinMain` diff: ours vs re3 (Phase 0.5, done by reading) ✅

Our `NOTSA_WinMain` (`source/app/platform/win/WinMain.cpp:393`) and re3's `WinMain`
(`refs/re3/src/skel/win/win.cpp:2012`) are **structurally the same** — good, our reversed boot is on the right track.
Same order: `SystemParametersInfo(FOREGROUNDLOCKTIMEOUT)` → `RsEventHandler(rsINITIALIZE)` → register window class →
`rsPREINITCOMMANDLINE` args → create window → init input → `rsRWINITIALIZE` → `rsCOMMANDLINE` args → `rsCAMERASIZE` →
screensaver/power/stickykeys → show+update window → load `gta_sa.set`/`gta3.set` controls → main loop.

Notable differences to keep in mind for the standalone port:
1. **Allocator:** re3 calls `InitMemoryMgr()` at the very top (`#ifdef USE_CUSTOM_ALLOCATOR`); we have no equivalent —
   our global `operator new` silently uses `CMemoryMgr` without init (see §5.2 / §5b). ← **first thing to fix.**
2. **Command line:** re3 uses `GetCommandLine()` + `CommandLineToArgv()`, then frees it. We use the CRT's `__argv`/
   `__argc` and then deliberately zero them (`__argc = 0; __argv = nullptr;`) as a "dirty fix for a crash in crt when
   freeing cmd line args" — a DLL-injection artifact. In standalone (our own exe) `__argv` is a normal CRT array, so
   this hack is likely unnecessary (but harmless). ⚠️
3. **Main loop:** re3's is a single `while (TRUE)` with a `RwInitialised` flag; ours is SA's `MainLoop` +
   `ProcessGameLogic` state machine (`gGameState`, `GAME_STATE_*`) — that's correct for SA, keep it. ✅
4. re3 re-inits mouse position (`RsMouseSetPos`) every loop iteration; we commented it out as "useless and annoying"
   (SA difference). Keep ours. ✅

## 6. Proposed standalone roadmap ❓

* **Phase 0 — entry point & storage (started):** real `WinMain`, shared bootstrap, config loading, and address-keyed
  storage for globals. *Done in this session (unverified build).*
* **Phase 1 — self-contained memory:** make `CMemoryMgr` initialize/behave in standalone; decouple global `new/delete`
  from the game allocator (or guarantee init order); audit static-initialization heap usage.
  * Template: re3's `USE_CUSTOM_ALLOCATOR` — see `MEMORY.md` §5b. Simplest option: don't overload the global
    allocator in standalone at all (use the CRT), at least until everything else works.
* **Phase 2 — eliminate `plugin::Call`:** reverse the remaining functions reachable from startup (or add explicit,
  clearly-marked stubs that fail loudly). Drive this from the actual startup call graph, not alphabetically.
* **Phase 3 — subsystems:** RenderWare (librw + C-API shim — see §3/§5b) / D3D9, windowing/input, filesystem, audio,
  save system.
* **Phase 4 — packaging:** exe name/assets/install flow, replace the ASI install flow in `contrib/install.py`.
* **Phase 0.5 — sanity checks against `refs/re3`:** walk our `NOTSA_WinMain` against re3's `WinMain` and note
  differences in `MEMORY.md`. ⚠️

Also cross-check all of this against the closest existing prior art, `refs/re3` (III) and `refs/reVC` (VC) —
complete standalone games built on the same engine family, plus `refs/librw` for RenderWare. ⚠️

## 7. Useful facts / commands

* Hook a global function (mod): `RH_ScopedGlobalInstall(Fn, 0xADDR)`; reversed classes: `RH_ScopedInstall`, etc.
* Hook IDs that matter here: `NOTSA_WinMain @ 0x748710`, `MessageLoop @ 0x746870`, both `.Locked = true`. ✅
* `docs/ai/TipsAndTricks.md` — aggregated reversing knowledge (enums, struct offsets, IDA quirks). ✅
* Grep helper for outstanding work:
  `Select-String -Path source -Pattern 'plugin::Call' -Recurse` .
