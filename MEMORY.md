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
* **Visual Studio 2022 Community 17.14** (`C:\Program Files\Microsoft Visual Studio\2022\Community`, instance
  `14ba42e0`) + BuildTools (`...\2022\BuildTools`, instance `67f0becd`) are present. MSVC toolset **14.44**
  (`_MSC_VER` 1944 ⇒ Conan `compiler.version=194`), x86 target exists, `vcvarsall.bat` works, `cl` compiles. ✅
* ❗ **Root problem: the VS instance is *incomplete*.** From `state.json` (`selectedPackages`):
  the ***.NET desktop development*** workload + the single component `VC.Tools.x86.x64` are installed, but
  **NOT** the **"Desktop development with C++" (`Workload.NativeDesktop`)** workload and **NOT** any **Windows SDK**.
  Consequences, all confirmed empirically:
  1. **No `windows.h` anywhere** ⇒ `cl /c "#include <windows.h>"` fails with **C1083**. Hard blocker for any build.
  2. **CMake's VS generator fails**: `-G "Visual Studio 17 2022"` → *"could not find any instance of Visual Studio"*.
     CMake only accepts *complete* instances, and ours is incomplete. (vswhere still lists it, hence the confusion.)
  3. `HKLM\SOFTWARE\Microsoft\VisualStudio\SxS\VS7` is empty.
  **Fix (needs admin, user action):** VS Installer → *Modify* VS Community → check **"Desktop development with C++"**
  → Install. That brings the Windows SDK **and** completes the instance (fixing CMake detection).
  One-shot command:
  ```powershell
  & "C:\Program Files (x86)\Microsoft Visual Studio\Installer\setup.exe" modify `
    --installPath "C:\Program Files\Microsoft Visual Studio\2022\Community" `
    --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended --quiet --norestart
  ```
* **Conan 2.33 / CMake 4.4 / Ninja 1.13** installed via `pip` (no admin). `conan profile detect` → msvc 194 (correct).
  CMake also comes from Conan's `tool_requires` (profile pins `cmake/4.2.0`). ✅
* ❗ **Profile mismatch:** repo profile `conan/profiles/windows-msvc.txt` asks `compiler.version=195` (**VS 2026**),
  this machine has VS 2022 (**194**) ⇒ configure with `python setup.py -s compiler.version=194` (or install VS 2026). ⚠️
* ❗ **Deps build from source.** The profile is `arch=x86, build_type=Debug, compiler.cppstd=23, runtime=static`;
  Conan Center has no matching prebuilt binaries (it only offered `compiler.version=193` alternatives, also absent),
  so SDL3/ogg/vorbis/libjpeg-turbo/spdlog/tracy/imgui get **built from source** — slow (tens of minutes) but fine. ⚠️
* ❗ **Use Conan's CMake, not the pip one.** The build dir is configured by Conan's `tool_requires` **cmake 4.2.0**
  (from `~/.conan2`). Re-configuring the same dir with pip's `cmake 4.4` (also on PATH) causes
  *"CMake Configure step failed / no CMAKE_CXX_COMPILER"*. ⇒ Always drive builds via `python setup.py ...` (or call
  Conan's cmake explicitly); don't mix versions on one build dir. ⚠️ *(hit this 2026-10-11.)*
* Not admin (VS Installer/SDK can't be run by the agent). Disk: C: ~54 GB free, E: ~61 GB free. ✅

### Build works (mod config) — ✅ verified 2026-10-11
* After the workstation was completed, the **mod build succeeds**:
  `python setup.py --build -s compiler.version=194` → **`bin/Debug/gta_reversed.asi`** (~47 MB),
  plus `gta_reversed.lib` / `.exp` / `.pdb` in the same folder. *(Expected path per `source/CMakeLists.txt`'s
  `RUNTIME_OUTPUT_DIRECTORY .../bin/${CMAKE_BUILD_TYPE}/`; on the VS generator `CMAKE_BUILD_TYPE` is empty but CMake
  appends the config, giving `bin/Debug/`.)* ✅
* ⚠️ The `python setup.py ...` wrapper reports **exit code 1 even on success** (PowerShell treats Conan's stderr
  output as errors; the `create_source_group` custom rule is also the last thing printed). To get a trustworthy code,
  use the direct `cmake --build build --config Debug` instead — it returns **0**. ✅
* All our new files compile cleanly in the mod config (`Bootstrap.cpp`, `StandaloneStorage.cpp`, `dllmain.cpp`,
  `WinMain.cpp`). ✅
* Deps are built from source (`GTASA_USE_SDL3=ON` by default ⇒ SDL3, plus ogg/vorbis/libjpeg-turbo/spdlog/tracy/imgui). ⚠️

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
* **Naming (be precise):** **librw** (`aap/librw`) is the actual *open-source RenderWare reimplementation* (the engine);
  **fakerw** is re3's *glue layer* over it (a C-API shim), **not** an implementation itself. Both are open source, but
  only **librw** is permissively licensed (**MIT**) — re3 (the game code) is not usable (DMCA'd). Proof: `fake.cpp`
  does `#include <rwcore.h>` → `#include <rw.h>` (librw) + `using namespace rw;`, and every `Rw*` function is a 1-liner
  over `rw::` (`RwV3dLength` → `length()`, `RwMatrixDestroy` → `mpMat->destroy()`). ✅
* **Version gap (important):** we are on **RW 3.7.0.2** (`source/game_sa/RenderWare/rw/rwversion.h`), while re3's fakerw
  targets the older **III/VC-era** RW API. ⇒ It can't be dropped in as-is; an **SA-flavored** shim is needed, covering
  SA's larger API surface (RpWorld/RpAtomic/RpGeometry/RpMatFX/RpSkin/RpHAnim/RtAnim/RpUVAnim/RpPTank/D3D9 exts…). ⚠️
* **librw builds with our toolchain** — verified: `cmake -G "Visual Studio 17 2022" -A Win32 -DLIBRW_PLATFORM=D3D9`
  → `librw.lib` (~3 MB). librw supports platforms NULL/GL3/**D3D9** (+PS2/Xbox). ✅ *(MIT license.)*
* **librw version matters:** `aap/librw` has only `master`, which is already the `rw::` namespaced C++ API (it stopped
  being the C API in commit `14c773d`, 2019-08-04). re3/reVC pin librw at **`5501c4fd…` (2021-08-19)** — the same
  C++ API. So librw + a shim is the path; do **not** expect a drop-in `RwCameraClear`. ✅

## 4. Scale of the remaining work (⚠️ measured, but re-verify)

* **~835 `plugin::Call*` uses across ~190 files.** These are **absolute-address calls into the original executable**.
  They cannot work in the standalone build (the original code isn't mapped there). This is *the* long-tail blocker. ⚠️
* Progress is tracked in `docs/ReversedClasses.md` and `docs/hooks.json`; upstream estimates ~50–60% reversed. ⚠️

## 5. Known blockers / gotchas for the standalone build 🚧

### 5a. Standalone boot progress (observed, 2026-10-11) ✅
The standalone `gta_reversed.exe` now boots (verified by running it and reading `logs/log.log`).
Sequence of crashes fixed so far, in order:
1. **Crash during hook installation** — `InjectHooksMain` → `CCopPed::InjectHooks` → `RHManager::InstallVirtual`
   → `VMTInfo::FindIndexOf` (C++ exception). *Cause:* `InstallVirtual` (and `RH_ScopedVirtualClass`) guarded on
   `NOTSA_STANDALONE_DUMP_HOOKS_ONLY` instead of `NOTSA_STANDALONE`, so the full standalone build still tried to
   read the original game's vtables. **Fixed** (`RHManager.cpp`, `ReversibleHooks.h`).
2. **Crash in the memory manager** — `CGame::InitialiseOnceBeforeRW` → `CdStreamInit` → `CMemoryMgr::MallocAlign`
   (`EIP 0x72F4C0`, i.e. the `plugin::Call` to the original game). *Cause:* `CMemoryMgr` falls back to `plugin::Call`
   when `MEMORY_MGR_USE_MEMORY_HEAP` is off, and the 2-arg `MallocAlign` overload called `0x72F4C0` unconditionally.
   **Fixed** — under `NOTSA_STANDALONE`, `Malloc`/`Free`/`Realloc`/`Calloc`/`MallocAlign`/`FreeAlign` now use the CRT.
3. **Reached game-data loading** — `CdStreamOpen: MODELS\GTA3.IMG`. It hangs here because there are no game assets.
   ⬅ **This is where the game data is needed.** Remaining `CMemoryMgr::InitScratchPad` (0x72F480) still calls the game
   but isn't reached (heap path is off). ⚠️
4. **Next expected blocker:** `RsRwInitialize` (rsRWINITIALIZE) → RenderWare (`plugin::Call` into the original exe).
   Needs a real RW implementation (librw + shim) — see §3/§5b/§6. ⚠️

**How to run it** (once assets exist): put the full GTA:SA assets (`models/`, `data/`, `audio/`, `anim/`, `movies/`,
`text/`, ...) next to `gta_reversed.exe` and run it with the assets as the working directory
(it opens relative paths like `MODELS\GTA3.IMG`). Logs + crash dumps land in `logs/log.log`, so a run always tells us
exactly where it died.

### 5b. Other blockers

1. **`plugin::Call` / `CallAndReturn` to absolute addresses.** Non-reversed code physically lives in `gta_sa.exe`.
   Standalone needs either (a) all of it reversed, or (b) a strategy to satisfy those calls. ~835 sites. ⚠️
2. **Global `operator new`/`delete` — actually NOT a problem in standalone.** ✅ *(corrected 2026-10-11)*
   `MemoryMgr.h` only enables the `CMemoryMgr`-backed global `operator new/delete` under `MEMORY_MGR_USE_MEMORY_HEAP`,
   and **that macro is never defined anywhere in the tree** ⇒ the `#ifdef MEMORY_MGR_OVERLOAD_NEW_DELETE` block in
   `MemoryMgr.cpp` (`operator new(uint32)`/`operator delete`, *not* guarded by standalone) is **dead code**.
   The *other* global overrides (`MemoryMgr.cpp:442+`, calling the game's `0x82119A`/`0x821195`/`0x8214BD`/`0x8213AE`)
   are `#ifndef NOTSA_STANDALONE`. ⇒ In the **standalone build the global `new`/`delete` is the plain CRT one**;
   no init-order hazard from `new`. (Those 4 are the *only* global `operator new/delete` definitions; every other
   `operator new` in the tree is a class member.) ✅
2b. **`CMemoryMgr`'s non-heap path calls the game — the real early blocker.** `CMemoryMgr::Malloc/Free/Realloc/Calloc/
   FreeAlign` fall back to `plugin::Call*(0x72F420 / 0x72F430 / 0x72F440 / 0x72F460 / 0x72F4F0)` when
   `MEMORY_MGR_USE_MEMORY_HEAP` is off (which is the case). In standalone those absolute calls crash. This is part of
   the broader `plugin::Call` problem (point 1), but it's hit *very* early (game init) ⇒ fix it first, eg. back
   `CMemoryMgr` with the CRT (`malloc`/`free`) like re3 does, or enable its memory-heap implementation. ⚠️
   * The `MEMORY_MGR_USE_MEMORY_HEAP` route is a dead end: never defined, its `Init()` is bit-rotted, and `CMemoryHeap`
     still has a `plugin::CallMethod(0x72E8E0)` stub. See roadmap Phase 1 for the CRT plan. ✅
   * Side note: `USE_GTASA_ALLOCATOR` is a CMake compile-definition that is **never referenced in the sources** — dead. ✅
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
| Global `operator new`/`CMemoryMgr` | The custom allocator is **opt-in** (`USE_CUSTOM_ALLOCATOR`). When OFF, `MemoryMgrMalloc`/`Free`/… just call `malloc`/`free`. When ON, `WinMain` calls `InitMemoryMgr()` as the **first** thing (`gMainHeap.Init(128MB)`). Our global `new/delete` is fine in standalone (see §5.2), but our `CMemoryMgr` still needs a CRT-backed path instead of `plugin::Call` (§5.2b). ✅ |
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

* **Phase 0 — entry point & storage (done):** real `WinMain`, shared bootstrap, config loading, address-keyed storage
  for globals, hook-framework fix. ✅ *Verified by building and running (2026-10-11).*
* **Phase 0.5 — verify boot:** the exe now boots to game-data loading (`MODELS\GTA3.IMG`). ✅
* **Phase 1 — self-contained memory (mostly done):** `CMemoryMgr::{Malloc,Free,Realloc,Calloc,MallocAlign,FreeAlign}`
  are CRT-backed under `NOTSA_STANDALONE`. ✅ Remaining: `InitScratchPad` (0x72F480, not reached yet). ⚠️
* **Phase 2 — eliminate `plugin::Call`:** reverse the remaining functions reachable from startup (or add explicit,
  clearly-marked stubs that fail loudly). Drive this from the actual startup call graph, not alphabetically.
  **Next concrete target:** `RsRwInitialize` / RenderWare init (see §5a.4).
* **Phase 3 — subsystems:** RenderWare (librw + C-API shim — see §3/§5b) / D3D9, windowing/input, filesystem, audio,
  save system.
* **Phase 4 — packaging:** exe name/assets/install flow, replace the ASI install flow in `contrib/install.py`.
* **Phase 0.5 — sanity checks against `refs/re3` (done):** walked our `NOTSA_WinMain` against re3's `WinMain`. ✅

Also cross-check all of this against the closest existing prior art, `refs/re3` (III) and `refs/reVC` (VC) —
complete standalone games built on the same engine family, plus `refs/librw` for RenderWare. ⚠️

## 7. Useful facts / commands

* Hook a global function (mod): `RH_ScopedGlobalInstall(Fn, 0xADDR)`; reversed classes: `RH_ScopedInstall`, etc.
* Hook IDs that matter here: `NOTSA_WinMain @ 0x748710`, `MessageLoop @ 0x746870`, both `.Locked = true`. ✅
* `docs/ai/TipsAndTricks.md` — aggregated reversing knowledge (enums, struct offsets, IDA quirks). ✅
* Grep helper for outstanding work:
  `Select-String -Path source -Pattern 'plugin::Call' -Recurse` .

## 8. Running & debugging the standalone build

* **Command:** build → `bin/Debug/gta_reversed.exe`. Run it from a directory that has the game's assets
  (or put the exe into the GTA:SA folder). It opens relative paths (`MODELS\GTA3.IMG`, …), so cwd matters.
* **Logs:** `logs/log.log` (next to cwd) + console. The unhandled-exception handler
  (`app_debug.cpp: WindowsExceptionHandler`) logs the exception code, registers and a **full call stack** —
  so every run tells us exactly where it died. This is our main debugging tool. ✅
* `--debug` on the command line makes it wait for a debugger (`CommandLine::s_WaitForDebugger`). ✅
* `gta-reversed.ini` (optional) configures the fast-loader (skip logos/intro, auto-load a save, …). ✅

### Windowed mode & RDP ⚠️ (matters for remote-desktop testing — user requirement)
* The project force-enables windowed mode (`NOTSA_WINDOWED_MODE` is a **mandatory** compile definition, see
  `source/CMakeLists.txt`; comment: "it must be enabled, otherwise shit's gonna crash").
* The window is created with `WS_OVERLAPPEDWINDOW` in `Win32_InitInstance` (`WinMain.cpp`), and
  `notsa::WindowedMode::bWindowed = true` by default (`WindowedMode.cpp`).
* ❗ **But** the mechanism that *actually forces D3D9 into windowed mode* is `WindowedMode::AdjustPresentParams()`,
  called from `HookDirect3DDeviceReplacerSA` (hooked at `0x7F6781`). In the **standalone** build that hook is **not
  installed** (`notsa::InjectWindowedModeHooks()` is `#ifndef NOTSA_STANDALONE`, and hooks are `NullHook` anyway), so
  **windowed mode is NOT currently enforced** in standalone. ⚠️ *Needs solving before we can render — likely by calling
  `AdjustPresentParams` directly once we own the D3D9 device creation (which is tied to the RenderWare replacement).*

## 9. RenderWare replacement — the wall (plan) 🧱

* **State:** the standalone exe boots to `RsRwInitialize` → `psGetMemoryFunctions` → `plugin::Call(0x745510)` and dies
  (access violation), because the original `gta_sa.exe` isn't there. Everything RW-related after that is the same
  wall. Verified by running with real game assets. ✅
* **Scale of RW in our tree:** `source/game_sa/RenderWare/**` = 88 files / ~31k lines, with ~1400 absolute-address
  references (967 unique) — it's all thunks. These are the *declarations* (SA RW 3.7 headers) + address-thunks. ✅
* **Approach (mirrors re3, but for SA):**
  1. **Provide librw** (MIT, builds under our toolchain — see §3/§5b).
     * **Decision (2026-10-11): vendored copy** at `extern/librw` (commit `18532c2e13efbc1aa43f0b00b4459831f9414de0`),
       MIT license kept, wired into CMake **standalone-only** (`add_subdirectory` + `librw::librw`), see §11. ✅
       ⚠️ **It must live outside `source/`** — `source/CMakeLists.txt` globs all of `source/**/*.h*`+`*.c*`, so putting
       librw inside would pull its sources (and `.hlsl`, which MSBuild compiles with FXC → `X4541`) into our target.
     `refs/` is gitignored, so we cannot reference `refs/librw` from the build.
  2. **Write an SA-flavored C-API shim** (our own "fakerw" for RW 3.7.0.2) and replace the address-thunks in
     `source/game_sa/RenderWare/rw/*.cpp` with real implementations backed by librw.
  3. **Risks:** SA's game code uses RW 3.7 struct *layouts* directly (`RwCamera`, `RwFrame`, `RpClump`, `RpAtomic`,
     `RpGeometry`, …). librw's structs (`rw::Camera`, …) may differ ⇒ bridging could require layout adaptation
     (`typedef rw::X RwX` like re3, or offset-compatible wrappers). This is the main technical unknown. ⚠️
  4. **Then:** D3D9 device creation must go through us so windowed mode / RDP works (§8) — call
     `WindowedMode::AdjustPresentParams` on present-parameter creation.

* **Effort:** this is the biggest single chunk of the standalone port (multi-session). Start from the *boot-critical*
  path (`RwEngineInit` → `RwEngineOpen` → stream/file system → world/texture loading), not alphabetically. ❓

## 10. Struct-compatibility: SA RW 3.7 vs librw (investigated 2026-10-11) 📐

Compared our SA RW 3.7 headers (`source/game_sa/RenderWare/rw/*.h`) with librw (`refs/librw/src/*.h`).

| Type | Verdict | Detail |
|---|---|---|
| `RwV2d`/`RwV3d`, `RwMatrix`, `RwObject`, `RwObjectHasFrame` | ✅ **identical** | eg. `RwMatrixTag` == librw `Matrix` (right/flags/up/pad1/at/pad2/pos/pad3) |
| `RpMesh`, `RpMorphTarget` | ✅ **same layout** | only field *names* differ (`indices`/`indices`, `parentGeom`/`parent`, `verts`/`vertices`) |
| `RwTexture` | ⚠️ **prefix-identical** | librw appends `inGlobalList` (`LLLink`) after `refCount` |
| `RwFrame` | ❌ **different** | SA: `…, modelling, ltm, objectList, …`; librw: `…, objectList, matrix, ltm, …` (matrices vs objectList swapped; names differ too) |
| `RwCamera` | ❌ **different (major)** | field order fully reordered; librw adds `devView`/`devProj`/`world`/`clump`/`startUpdateCB` |
| `RwRaster` | ❌ **different** | different order **and** field types: librw uses `int32 type/flags/privateFlags/format` + a `platform` field; SA packs `u8 cType/cFlags/privateFlags/cFormat` |
| `RwImage` | ❌ **different** | librw inserts `bpp` after `depth` |
| `RpMaterial` | ❌ **different** | SA: `texture, color, pipeline, surfaceProps, refCount(i16)+pad`; librw: `texture, color, surfaceProps, pipeline, refCount(i32)` |
| `RpAtomic` | ❌ **different** | SA has `repEntry`, `renderCallBack`, `interpolator`, `renderFrame`; librw has `pipeline`, `renderCB`, `world`, `originalSync` |
| `RpClump` / `RpWorld` / `RpGeometry` | ⚠️ not exhaustively compared | very likely differ (same pattern) |

**⇒ librw is NOT layout-compatible with SA's RW 3.7. The `typedef rw::X RwX` trick (re3-style) needs adaptation.**

**Good news — the layout-sensitive surface is small.** In `source/{game_sa,app}`:
* Direct access to divergence-prone RW *fields*: **~115 sites** (sampled), so realistically a few hundred total.
  Most code touches RW only through *functions* (`RwCameraClear`, `RpAtomicGetGeometry`, …), which don't care about layout. ✅
* So the coupling is bounded; a `RwHelper.h`-style **accessor** layer (already an established pattern in the repo!) can
  abstract the differing fields for both builds.

**Key constraint for any approach:** the **mod build needs SA's exact RW layouts** (it runs inside the real game). So
the standalone (librw-backed) RW must be `#ifdef NOTSA_STANDALONE`-split from the mod's SA RW. The ~115 field-access
sites must then work for *both* ⇒ migrate them to accessors.

**Approach options (decision pending):**
* **(A) re3-style, guarded (recommended):** standalone adopts librw types; mod keeps SA RW; add accessors for the
  differing fields. Keeps librw pristine/updatable; proven pattern. Biggest work = the ~300 RW thunks + accessor migration.
* **(B) Fork librw's structs to SA's layouts:** closer to "drop-in", but invasive to librw internals (streaming, D3D9
  native rasters) and still needs the header split. Riskier.
* **(C) Boot-spike first:** implement only the RW calls needed to get *past* engine init (black window), to validate
  the whole pipeline, then decide. Smallest first step. ❓

## 11. RW integration — decisions & worklist (2026-10-11) 🛠️

**Decisions (by the user):** approach **(A)** (re3-style, `#ifdef NOTSA_STANDALONE`), and **vendor librw** now.

**Vendoring (done):**
* librw copied to **`extern/librw`** (outside `source/`!), commit `18532c2e13efbc1aa43f0b00b4459831f9414de0`, MIT.
  Details/rationale: `extern/README.md`.
* `source/CMakeLists.txt`: standalone-only `add_subdirectory("${CMAKE_SOURCE_DIR}/extern/librw" …)` +
  `set(LIBRW_PLATFORM "D3D9" CACHE … FORCE)` + `target_link_libraries(... librw::librw)`.

**Boot-critical RW shim worklist** (the minimal set to get past `RsRwInitialize` — `platform.cpp`):
1. `RwEngineInit(memFuncs, 0, arena)` → `rw::Engine::init(memFuncs)`
2. `RwEngineOpen(&openParams)` → `rw::Engine::open(...)`
3. `RwEngineStart()` / `RwEngineStop()` / `RwEngineClose()` / `RwEngineTerm()` → `rw::Engine::{start,stop,close,term}`
4. Device/video-mode: `psSelectDevice`, `RwEngineSetVideoMode`, `RwEngineGetNumVideoModes`, `RwEngineGetVideoModeInfo`,
   `RwEngineGetCurrentVideoMode` → librw's `Engine::*SubSystem/VideoMode*` + `Device::system(DEVICE…)`
5. `RwD3D9DeviceSupportsDXTTexture()` (for `psNativeTextureSupport`)
6. `RwTextureSetMipmapping` / `RwTextureSetAutoMipmapping` → `rw::Texture::setMipmapping/…`
7. Stream/file system + resource arenas (`RwStreamOpen`, file callbacks) — librw has its own `Stream`/`StreamFile`.
   SA's `psInstallFileSystem()` is a no-op, so librw's default file system should do.

Also: `RwEngineInit` is called with SA's `RwMemoryFunctions` (from `psGetMemoryFunctions`, currently `plugin::Call`).
In standalone that must return a real table — point it at `CMemoryMgr` (CRT) or librw's own (`RwOsGetMemoryInterface`). ⚠️

**Status:** librw integrated into the build (compiles); the shim (actually replacing the thunks) is **not started**. ⚠️
