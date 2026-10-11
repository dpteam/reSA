# RESUME.md — Session snapshot

> **How to use:** at the *start* of a session, read this + `MEMORY.md` + `PROGRESS.md`. Rewrite at the end of a session.
> Keep it actionable — details live in `MEMORY.md`/`HISTORY.md`/`PROGRESS.md`.

---

## Last updated
2026-10-12 (session 2, night)

## TL;DR — where we are 🎉
**The standalone game `reSA.exe` RUNS and RENDERS.** It boots, creates its window, and **draws the frontend in 2D**
(on screen you can see the **VINEWOOD** splash/background). It **does not crash** (stable, ~forever).

Big wins this session:
1. **RenderWare is provided by `librw`** (vendored, MIT) instead of `gta_sa.exe`, via a C-style **bridge**
   (`source/game_sa/RenderWare/RwBridge.{h,cpp}`, the only TU that includes librw's `<rw.h>`).
2. The game runs the boot chain → `MainLoop`/`ProcessGameLogic` → `FRONTEND_LOADING/LOADED/IDLE` →
   `CMenuManager::Process`/`DrawFrontEnd`, and its draw calls **actually reach librw/D3D9**.

### Known gaps (all documented in `IDEAS.md`)
* ❌ **Text doesn't render** — `CFont::RenderFontBuffer` and `CFont::ProcessCurrentString` are **not reversed** (upstream neither);
  they're no-ops in standalone. Menus/HUD will be text-less until implemented (check `refs/reVC` `src/text/`).
* ❌ **No sound** — `CAESoundManager::Service` is skipped in standalone (sounds had a garbage `m_AudioEntity`).
* ⚠️ **`RwIm3D*`** (effects/lines) still call into the original exe — not bridged yet.
* ⚠️ RxPipeline cosmetics (car env-map / custom building) disabled; ImGui debug UI disabled.
* ⚠️ **Windowed mode not forced yet** — needed for RDP testing.

## Build state
* ✅ Both configs build (VS 2022, MSVC 194, SDK 10.0.26100), **standalone exe is `bin/Debug/reSA.exe`** (target
  `gta_reversed`); mod is `bin/Debug/gta_reversed.asi`.
* Build cmd: `python setup.py --build -o standalone=True -s compiler.version=194`
  (⚠️ the wrapper prints `Exited with code 1` **even on success** — verify that a `gta_reversed.vcxproj -> ...reSA.exe`
  line appears and there are no `error C/LNK`).
* **Run/test:** `powershell -ExecutionPolicy Bypass -File .\run_test.ps1 -Seconds 60 -Shot shots\menu.png`
  (helper added this session: auto-dismisses MSVC CRT debug dialogs by clicking **"Пропустить"/Ignore**, captures a
  screenshot, prints the log tail, and kills the process). Assets are already in `bin/Debug`; run with it as cwd.
  Log: `bin/Debug/logs/log.log` (full stack on crash).
* ❗ Always pass `-s compiler.version=194` (repo profile says 195 = VS 2026).
* ❗ Don't reconfigure `build/` with pip's cmake; use Conan's (`python setup.py`). See `MEMORY.md` §2.

## What was done in THIS session (all committed, each verified by running)
**1. librw integration** → `extern/librw` (outside `source/`), CMake adds it only under `GTASA_STANDALONE`.
   **`RwBridge.{h,cpp}`** exposes the RW API as plain C functions; the SA RW thunks in `rwplcore.cpp`/`rwcore.cpp`/
   `rpworld.cpp` get `#ifdef NOTSA_STANDALONE` bridge bodies. SA-layout-dependent macros are `#undef`/redefined to bridge
   calls (`RwCameraGet/SetRaster`, `Get/SetZRaster`, `GetFrame`, `RwRasterGetWidth/Height/Depth`, `RpLightSetFlags/SetFrame`,
   `RwIm2DGetNear/FarScreenZ`). **Key gotcha:** SA's `RwRenderState` and librw's `RenderState` are **ordered differently** →
   `RwBridge::TranslateRenderState` maps them (unknown states are ignored). `RwRenderStateSet/Get` + `RwIm2D*` macros route
   through the bridge.

**2. Boot-chain fixes (each found from `logs/log.log` and fixed):**
   - `psGetMemoryFunctions` → CRT table; `PluginAttach` → `rwbridge::RegisterCorePlugins()`.
   - `CMemoryMgr` → CRT under standalone; `InjectHooksMain` uses NullHook.
   - `CGamma::Init` disabled; `GetMillisecondTime` → `OS_TimeMS()`.
   - **SurfaceInfos_c::LoadAdhesiveLimits** implemented (`data/surface.dat`).
   - **`CDecisionMakerTypes`/`CDecision`** (GetInstance/LoadEventIndices/Add/RemoveDecisionMaker/SetDefault/Set) implemented (best-effort).
   - `CCarFXRenderer`/`CCustomBuildingRenderer` Initialise no-op (RxPipeline); `D3DResourceSystem` all no-op.
   - `RwD3D9GetCaps` → librw (`rw::d3d9::getD3D9Caps`, added to librw + logged in `extern/README.md`).
   - `Sprite2d`/`Sprite` screen-Z → `RwIm2DGetNear/FarScreenZ()`.
   - `CAEFrontendAudioEntity::{AddAudioEvent,UpdateParameters}` → use the reversed (untested) bodies.
   - `CAEWMADecoder::WMCreateSyncReader` absolute ref → `StaticRef`; `CAERadioTrackManager::Service` + `CAESoundManager::Service` skipped in standalone; `tSoundPlayParams::AudioEntity` default-inits to nullptr.
   - `C_PcSave::GenerateGameFilename`: clamp `sprintf_s` size to `MAX_PATH` (was ~263 → stack corruption).
   - `reSA.exe` output name + window icon (`app.rc` + `app.ico`).

## Next steps (in order)
1. **Text rendering** — implement `CFont::RenderFontBuffer` + `ProcessCurrentString` (see how `PrintChar`/`CSprite2d` are used;
   check `refs/reVC` `src/text/Text.cpp`). Without it the menu/HUD are unreadable.
2. **Menu navigation / input** — confirm the menu reacts (the VINEWOOD background shows; text is missing).
3. **Start a new game / world load** — IMG/IPL/IDE streaming, `CGame::InitialiseRenderWare` already done.
4. **Windowed mode for RDP** (see `MEMORY.md` §8).
5. **Audio** (fix `m_AudioEntity` usage; then re-enable `CAESoundManager::Service`).
6. **`RwIm3D*` bridging** (effects/lines) — next big RW task.

## Key references / where to look
* `PROGRESS.md` — boot-chain status table (updated this session).
* `MEMORY.md` §2 (env), §3 (RenderWare/librw), §5a (boot progress + crash list), §8 (run/dbg), §9–§11 (RW replacement).
* `IDEAS.md` §4 — backlog + all standalone stubs/gaps (text, audio, RwIm3D, RxPipeline, ImGui, …).
* `refs/reVC` / `refs/re3` — **closest prior art**; when something isn't reversed, check these for a direct/near equivalent
  (`src/audio/`, `src/text/`, `src/renderer/`, `src/fakerw/`). Rule: read-only, reimplement in our own style (see `refs/README.md`).

## Commands cheat-sheet
```powershell
# build (standalone). The wrapper fakes exit 1 - check for the reSA.exe line & no "error C/LNK".
python setup.py --build -o standalone=True -s compiler.version=194
# run + auto-skip CRT dialogs + screenshot + log tail
powershell -ExecutionPolicy Bypass -File .\run_test.ps1 -Seconds 60 -Shot shots\menu.png
```
