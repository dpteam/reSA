# HISTORY.md — Work log

> Append-only. Newest entries go **on top**. One entry per work session / logical change.
> For the upstream history, use `git log`. This file records *our* (fork-specific) work and reasoning.

Legend: ✅ verified (built/tested) · ⚠️ unverified (no toolchain) · 🚧 in progress

---

## 2026-10-12 (session 2, night) — the game RUNS, renders the frontend (menu) with real background pictures ✅

**Milestone:** `reSA.exe` boots, renders the **frontend** (menu) — real background pictures from the frontend TXD
(VINEWOOD, desert, palms, …) are visible on screen — and the menu is **navigable** (keyboard/SDL input works; pressing
**Enter** changes the screen). It does **not** crash. ⚠️ Two visible issues: (a) everything is drawn in the **top-left
corner** of the window (viewport/present bug), (b) **no text** is drawn (unreversed `CFont::RenderFontBuffer` etc.).

> Correction: earlier I thought a "desert/buildings/sky" frame meant the game had started a 3D world — the user pointed
> out those are **menu background pictures** from the frontend TXD. Still the menu. Starting an actual (new) game is
> **not** done yet.

**Key changes (all verified by running):**
- **Render states + Im2D bridged** (`RwBridge.cpp` `TranslateRenderState` + `RwRenderStateSet/Get`, `RwIm2D*` macros in
  `rwplcore.h`). SA vs librw render-state enums differ in order → translated.
- Boot fixes: `SurfaceInfos_c::LoadAdhesiveLimits` (parse `surface.dat`), `CDecisionMakerTypes`/`CDecision` (best-effort),
  `RwD3D9GetCaps` (via librw `rw::d3d9::getD3D9Caps`), sprite screen-Z, `D3DResourceSystem` no-op,
  `CAEFrontendAudioEntity::{AddAudioEvent,UpdateParameters}` (reversed bodies), `CAEWMADecoder` `StaticRef`,
  `CAESoundManager::Service`/`CAERadioTrackManager::Service` skipped, `tSoundPlayParams::AudioEntity` default-init,
  file `CFont`/audio stubs.
- **`C_PcSave::GenerateGameFilename`** stack-corruption fix (sprintf_s size → `MAX_PATH`).
- Helper `run_test.ps1` (auto-dismiss CRT dialogs, screenshot, log tail).

**References added:** `refs/AdditionalSources/*` — `SOME_GTA_SA_Sources` (partial SA engine reverse; has
`CSprite2d::AddToBuffer`), `PartialSAMobileCode` (`pipes/` = RxPipeline), `SAMP` (future multiplayer). See `refs/README.md`.

**Findings (text):** `refs/re3`/`refs/reVC` `CFont` use the same *idea* as SA (temp vertex + index buffers via
`CSprite2d::AddToBuffer`); their `PrintChar` UV math matches SA's reversed `PrintChar`. Still need SA's
`AddToBuffer`/`RenderFontBuffer`/`ProcessCurrentString`.

---

## 2026-10-12 (session 2) — RenderWare via librw; the game reaches MainLoop ✅

**Milestone:** the standalone `reSA.exe` now initializes RenderWare (via **librw**) and enters the game loop
(`MainLoop`/`ProcessGameLogic`), loading real assets (font/HUD/skin TXDs).

**How:** vendored **librw** (`extern/librw`, MIT, commit `18532c2e…`) + a C-style **bridge**
(`source/game_sa/RenderWare/RwBridge.{h,cpp}`, the only TU that includes librw's `<rw.h>`). RW functions in
`rwplcore/rwcore/rpworld/...` got `#ifdef NOTSA_STANDALONE` bridge bodies; SA-layout macros
(`RwCameraGet/SetRaster`, `RwRasterGetWidth/Height/Depth`, `RpLightSetFlags/Frame`, `RwIm2DGetNear/FarScreenZ`)
were overridden under standalone to route through the bridge. See `MEMORY.md` §9–§11.

**Boot chain progressed (each step verified by running, fixing one crash at a time):**
`psGetMemoryFunctions` → `RwEngineInit/Open/Start` → `PluginAttach` → `psSelectDevice` → `RsRwInitialize` →
`RwCameraCreate/FrameCreate/RasterCreate/MatrixCreate` + `RwCameraClear` → `RpWorldCreate`/`AddCamera`/`AddLight`/
`RpLightCreate`/`RwFrameRotate` → `CFont::Initialise` (**TXD + textures loaded**) → `CHud::Initialise` →
`CPlayerSkin::Initialise` → `CPostEffects` → `CGame::InitialiseRenderWare` **complete** → `CGamma::Init` →
**`MainLoop`** → `CLoadingScreen::Init` → `CGame::InitialiseEssentialsAfterRW` (after stubbing the RxPipeline-based
custom renderers) → **now at** `CGame::InitialiseCoreDataAfterRW` → `SurfaceInfos_c::Init` (crash: `plugin::CallMethod`
to `0x55D0E0`).

**Other fixes:** disabled ImGui debug/tools UI in standalone (`UIRenderer`, `DebugModules`) — revisit later;
`GetMillisecondTime` → `OS_TimeMS()`; `CGamma::Init` disabled; `_rwD3D9DeviceSet/GetRestoreCallback` stubbed;
`CCarFXRenderer`/`CCustomBuildingRenderer` `Initialise` no-op (RxPipeline cosmetics).

**Product changes:** standalone binary renamed to **`reSA.exe`** (`OUTPUT_NAME reSA`); added **window icon**
(`app.rc` + `data/Icons/app.ico`, `IDI_MAIN_ICON=1042`).

**Docs:** `PROGRESS.md` (new, boot-chain table), `IDEAS.md` (new, backlog incl. `refs/gtasa10us` 1.0 US exe/PDB/CrashList,
upstream-PR porting policy, "bring back ImGui", "RxPipeline on librw"). `MEMORY.md` §5a/§9–§11 updated. `RESUME.md`
rewritten.

**Verification:** standalone built (`bin/Debug/reSA.exe`) and run repeatedly; each blocker above found in
`logs/log.log` and fixed. ⚠️ Mod config not rebuilt after shared-file changes (all changes are `#ifdef NOTSA_STANDALONE`,
mod path unchanged) — a mod rebuild is still pending.

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

---

### Follow-up (same day, 5): ran with real game assets → hit the RenderWare wall

* User dropped the GTA:SA assets (**without** `gta_sa.exe`, by design) into `bin/Debug/` (`models/`, `data/`, `audio/`,
  `anim/`, `movies/`, `text/`, `models/gta3.img`, `data/gta.dat`).
* Ran `gta_reversed.exe` from there — it now gets past `CdStreamOpen: MODELS\GTA3.IMG`, then crashes with
  **`0xC0000005` at `EIP=0x00745510`**: `RsRwInitialize` → `psGetMemoryFunctions` → `plugin::CallAndReturn<…,0x745510>`.
  ⇒ **No RenderWare in standalone** (the original RW lives in `gta_sa.exe`). This is the expected wall.
* **Feasibility check for librw:** cloned `aap/librw` builds cleanly with our exact toolchain
  (`-A Win32 -DLIBRW_PLATFORM=D3D9`) → `librw.lib` (~3 MB). **MIT-licensed.**
* **Clarified (user raised it):** *librw* is the open-source RW reimplementation; *fakerw* is re3's C-API shim over it,
  targeting the older III/VC-era API. We're on RW **3.7.0.2**, so we need an SA-flavored shim (or equivalent). Details
  + the plan + the vendoring decision: `MEMORY.md` §3 and the new §9.
* **No code changes** in this step (Ran/diagnosed + built librw outside the tree). Next step is the RW integration,
  which needs a decision on how to source librw (§9.1).

---

### Follow-up (same day, 6): struct-compatibility investigation (SA RW 3.7 vs librw)

User chose: vendor librw into the repo, **but first** check struct compatibility.

* Compared SA's RW 3.7 headers with librw (`MEMORY.md` §10):
  * **Identical:** `RwV2d/V3d`, `RwMatrix`, `RwObject`, `RwObjectHasFrame`, `RpMesh`, `RpMorphTarget` (names aside).
  * **Differs:** `RwFrame`, `RwCamera`, `RwRaster`, `RwImage`, `RpMaterial`, `RpAtomic`. `RwTexture` only appends a field.
  * ⇒ librw is **not layout-compatible**; the re3-style `typedef rw::X RwX` needs adaptation.
* **Encouraging:** direct access to divergence-prone RW fields is only **~115 sites** (sampled) — the code mostly goes
  through RW *functions*, so the layout-sensitive coupling is bounded and can be abstracted via `RwHelper.h` accessors.
* **Constraint:** the mod build needs SA's exact layouts ⇒ the librw-backed RW must be `#ifdef NOTSA_STANDALONE`-split.
* Wrote `MEMORY.md` §10 with the table, the constraint, and three approach options (A: re3-style guarded,
  B: fork librw layouts, C: boot-spike first). **Decision pending** before any code.

**Verification:** read-only investigation; no build/run.

---

### Follow-up (same day, 7): vendored librw + wired it into the (standalone-only) build

User chose approach **(A)** (re3-style, `#ifdef NOTSA_STANDALONE`) and to **vendor librw** now.

* Vendored librw to **`extern/librw`** (commit `18532c2e…`, MIT, license kept). Excluded `skeleton/`, `tools/*`
  (kept only the `tools/CMakeLists.txt` stub the build needs), CI/premake files. Documented in `extern/README.md`.
* ❗ **Trap #1:** the first attempt put it at `source/libs/librw` — wrong, because `source/CMakeLists.txt` globs all of
  `source/**/*.h*`+`*.c*`, so librw's sources **and its `.hlsl` shaders** got added to our target (MSBuild then ran FXC
  → `error X4541`). Moved it **outside `source/`** to `extern/`.
* ❗ **Trap #2:** setting `LIBRW_PLATFORM` as a normal variable didn't take (librw's `set(... CACHE)` won) → it built
  the NULL backend. Fixed with `set(LIBRW_PLATFORM "D3D9" CACHE STRING "…" FORCE)`.
* ❗ **Trap #3:** re-configuring the build dir with pip's `cmake 4.4` (build dir was made by Conan's cmake 4.2) broke
  compiler detection. Always build via `python setup.py …` (Conan's cmake).
* `source/CMakeLists.txt`: standalone-only
  `add_subdirectory("${CMAKE_SOURCE_DIR}/extern/librw" "${CMAKE_BINARY_DIR}/extern/librw")` + link `librw::librw`.
* ✅ **Verified:** standalone config now builds `librw` with `LIBRW_PLATFORM=D3D9` (`librw.lib`, ~4 MB) and links it into
  `bin/Debug/gta_reversed.exe`. The exe still runs and still stops at the RenderWare wall (expected — the RW thunks
  aren't replaced yet). Mod build unaffected (librw only added under `GTASA_STANDALONE`).

**Next:** start the RW shim (the boot-critical worklist in `MEMORY.md` §11).

**Verification:** built the standalone config (Conan/cmake 4.2), confirmed D3D9 + link, ran the exe (same known crash).
