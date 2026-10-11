# RESUME.md — Session snapshot

> **How to use:** at the *start* of a session, read this + `MEMORY.md` + `PROGRESS.md`. Rewrite at the end of a session.
> Keep it actionable — details live in `MEMORY.md`/`HISTORY.md`/`PROGRESS.md`.

---

## Last updated
2026-10-12 (session 2; before compaction)

## TL;DR — where we are
**The standalone game `reSA.exe` now runs deep into startup: through RenderWare init and *into the game loop*.**

The big win of this session: **RenderWare is provided via library `librw`** (vendored, MIT) instead of the original
`gta_sa.exe`, so the standalone build can actually initialize the engine, create the camera/world/lights, load font/
HUD/skin textures (real assets!), and enter `MainLoop`/`ProcessGameLogic`.

Current blocking point (last verified run):
```
MainLoop -> ProcessGameLogic (GAME_STATE_FRONTEND_LOADING)
  -> CGame::InitialiseCoreDataAfterRW (Game.cpp:568)
    -> SurfaceInfos_c::Init -> SurfaceInfos_c::LoadAdhesiveLimits (SurfaceInfo_c.cpp:255)
      -> plugin::CallMethod<0x55D0E0, SurfaceInfos_c*>   <-- CRASH (0x55D0E0 = original exe)
```
⇒ Next blocker = `SurfaceInfos_c` (surface data loading). Need to reverse/implement `LoadAdhesiveLimits` and the rest
of `SurfaceInfos_c::Init` (they use `plugin::Call*` into `gta_sa.exe`). Files: `source/game_sa/SurfaceInfo_c.cpp`,
`source/game_sa/SurfaceInfos_c.cpp`.

## Build state
* ✅ Both configs build (VS 2022, MSVC 194, SDK 10.0.26100), **standalone exe is `bin/Debug/reSA.exe`** (target
  `gta_reversed`); mod is `bin/Debug/gta_reversed.asi`.
* Build cmd: `python setup.py --build -o standalone=True -s compiler.version=194`
  (⚠️ the wrapper prints `Exited with code 1` **even on success** — verify via `cmake --build build --config Debug` = 0,
  or just check that `skipped`+`built`+`gta_reversed.vcxproj -> ...reSA.exe` lines appear and no `error C/LNK`).
* Run: put assets in `bin/Debug` (already there) and launch `bin/Debug/reSA.exe` from that dir. Log: `bin/Debug/logs/log.log`
  (full stack on crash). **Always run after building** — it's the fastest feedback loop.
* ❗ Always pass `-s compiler.version=194` (repo profile says 195 = VS 2026).
* ❗ Don't reconfigure `build/` with pip's cmake; use Conan's (`python setup.py`). See `MEMORY.md` §2.

## What was done in THIS session (all committed, each verified by running)
RenderWare replaced by librw + a C-style bridge, then worked through the boot chain step by step:
1. **librw vendored** → `extern/librw` (outside `source/`!). CMake adds it only under `GTASA_STANDALONE`. See `extern/README.md`.
2. **`source/game_sa/RenderWare/RwBridge.{h,cpp}`** — the ONLY TU that includes librw's `<rw.h>` (PCH disabled for it).
   Exposes the RW API the game needs through plain C functions/POD. All RW functions in `rwplcore.cpp`/`rwcore.cpp`/
   `rpworld.cpp` etc. got `#ifndef NOTSA_STANDALONE` (original thunks) / `#ifdef NOTSA_STANDALONE` (bridge) bodies.
3. **Macros that read SA struct offsets were overridden** under `NOTSA_STANDALONE` to call the bridge:
   `RwCameraGet/SetRaster`, `Get/SetZRaster`, `GetFrame`, `RwRasterGetWidth/Height/Depth` (in `rwcore.h`);
   `RpLightSetFlags/SetFrame` (in `rpworld.h`); `RwIm2DGetNear/FarScreenZ` (in `rwplcore.h`). This is the pattern to follow
   for any other SA-layout-dependent accessor (see `MEMORY.md` §10).
4. **Fixed `psGetMemoryFunctions`** (standalone → CRT-backed `RwMemoryFunctions` table using `CMemoryMgr`).
5. **`PluginAttach`** standalone → `rwbridge::RegisterCorePlugins()`.
6. **Debug/tools UI (ImGui) disabled in standalone** (`UIRenderer`, `DebugModules::CreateModules` no-op, `AESoundManager`
   debug hook gated). Revisit later — `IDEAS.md`.
7. **`CGamma::Init` disabled in standalone** (no D3D9 device/caps via librw). Gamma is optional.
8. **`GetMillisecondTime`** (Timer.cpp) → `OS_TimeMS()` in standalone.
9. **`CCarFXRenderer::Initialise` / `CCustomBuildingRenderer::Initialise`** → no-op in standalone (they use the classic
   `RxPipeline` API librw lacks; cosmetic). `RxPipeline*` is a big unported subsystem — see `IDEAS.md`.
10. **`reSA.exe`** output name + **window icon** (`source/app/platform/win/app.rc` + `app.ico`, id `IDI_MAIN_ICON=1042`).

## Files created/changed this session (key ones)
* `extern/librw/**` (vendored), `extern/README.md`, `.gitignore` (`/refs/*`), `source/CMakeLists.txt` (librw + `reSA` name + `.rc`).
* `source/game_sa/RenderWare/RwBridge.{h,cpp}`, `app.rc`, `app.ico`.
* `Base.h` (`StaticRef` standalone storage already existed; refs to bridge), `rwplcore.{h,cpp}`, `rwcore.{h,cpp}`,
  `rpworld.{h,cpp}`, `rwtexdict.cpp`, `platform.cpp`, `app.cpp`, `dllmain.cpp`, `WinMain.cpp`, `Timer.cpp`, `Gamma.cpp`,
  `PostEffects.cpp`(no—read only), `UIRenderer.cpp`, `DebugModules.cpp`, `AESoundManager.cpp`, `CarFXRenderer.cpp`,
  `CustomBuildingRenderer.cpp`, `D3DResourceSystem.cpp`.
* Docs: `PROGRESS.md` (boot chain table), `IDEAS.md`, `MEMORY.md` §5a/§9/§10/§11, `HISTORY.md`, this file.

## Next steps (in order)
1. **Fix `SurfaceInfos_c`** (current crash). Check `SurfaceInfo_c.cpp:255` + `SurfaceInfos_c::Init` — implement with CRT/
   own data instead of `plugin::CallMethod(0x55D0E0)`. This is CGame init core-data; likely needs `data/surface.dat`
   (present in assets).
2. Continue `CGame::InitialiseCoreDataAfterRW` (Game.cpp:565): `CTempColModels::Initialise`, `gHandlingDataMgr.Initialise`,
   `CPedStats::Initialise`, `CTimeCycle::Initialise`, `CPopCycle::Initialise`, `CVehicleRecording::InitAtStartOfGame`,
   `AudioEngine.Initialise`, `g_breakMan.Init`, `g_boneNodeMan.Init`, `g_ikChainMan.Init`.
3. Then `ProcessGameLogic` GAME_STATE_FRONTEND_LOADED → IDLE, then the real frontend/world load (streaming, IMG/IPL/IDE).
4. **Windowed mode / RDP**: not yet enforced in standalone (see `MEMORY.md` §8) — needed for remote-desktop testing.
   Likely: call `notsa::WindowedMode::AdjustPresentParams` on present-param creation once we own the D3D9 device.

## Open questions / decisions
* RxPipeline effects (car env-map, custom building): reimplement on librw pipelines later (`IDEAS.md`). Now stubbed.
* Bring back ImGui debug UI in standalone later (`IDEAS.md`).
* `plugin::Call` sites remain ~800+; the ones on the *boot path* are what matter now (see `MEMORY.md` §5a).

## Key references
* `PROGRESS.md` — boot-chain table (what's passed / current / next).
* `MEMORY.md` §2 (env), §3 (RenderWare/librw), §5a (boot progress + crash list), §8 (run/dbg + windowed-mode), §9–§11
  (RW replacement plan, struct compat, integration worklist).
* `IDEAS.md` §4 — `refs/` (re3/reVC/librw + `refs/gtasa10us` 1.0 US exe/pdb/CrashList).
* `refs/README.md` — what's in `refs/` and rules (read-only, no code copying).

## Commands cheat-sheet
```powershell
# build (standalone)
python setup.py --build -o standalone=True -s compiler.version=194
# trustworthy exit code:
cmake --build build --config Debug
# run (from assets dir) + read log
cd bin\Debug; .\reSA.exe; Get-Content logs\log.log
```
