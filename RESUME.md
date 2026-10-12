# RESUME.md — Session snapshot

> **How to use:** at the *start* of a session, read this + `MEMORY.md` + `PROGRESS.md`. Rewrite at the end of a session.
> Keep it actionable — details live in `MEMORY.md`/`HISTORY.md`/`PROGRESS.md`.

---

## Last updated
2026-10-12 (session 2, night)

> ### ✅ Before compacting the NEXT session (checklist — do this while there's budget)
> The user wants a "hand-off" state each session so work can continue after compaction. Before ending a session, make sure:
> 1. **`RESUME.md`** — rewritten (TL;DR, current state, blockers, next steps, commands). *This file.*
> 2. **`PROGRESS.md`** — boot-chain table updated with what's ✅/❌/⏳.
> 3. **`HISTORY.md`** — one entry for the session (what changed + how it was verified).
> 4. **`MEMORY.md`** — new findings (structs/layouts/refs) that shouldn't be lost.
> 5. **`IDEAS.md`** — anything new that was deferred (stubs, gaps, future features).
> 6. **`git`** — everything committed (clean tree), with a message that says *how it was verified*.
> 7. Nothing essential should live only in the chat log.

## TL;DR — where we are 🎉
**The standalone game `reSA.exe` RUNS, renders the frontend (menu), and is stable at ~60 FPS.**

* Boots → `MainLoop`/`ProcessGameLogic` → `FRONTEND_LOADING/LOADED/IDLE` → `CMenuManager` — real frontend TXD
  background pictures (VINEWOOD, …) are visible, the menu is **navigable** (keyboard/SDL input works; **Enter**
  changes screens). No crash. RenderWare is **librw** via the C-style **bridge** (`source/game_sa/RenderWare/RwBridge.*`).
* A per-second **FPS log** is in `logs/log.log` (standalone; no on-screen text yet). Observed ~60 FPS in the menu.

### Known gaps (all in `IDEAS.md`)
* ❌ **Text doesn't render** — `CFont::RenderFontBuffer`/`ProcessCurrentString`/`CSprite2d::AddToBuffer` are **not
  reversed** (upstream neither); no-ops in standalone. The menu is therefore unreadable. **Biggest gameplay blocker.**
  Reference: `refs/reVC`/`refs/re3` `src/renderer/Font.cpp` (same idea as SA) + `refs/AdditionalSources/SOME_GTA_SA_Sources`
  (has `CSprite2d::AddToBuffer`).
* ⏳ **Starting an actual game / world load** — not done yet (the menu works, but no new-game/load reached).
* ❌ **No sound** — `CAESoundManager::Service` skipped in standalone (sounds had a garbage `m_AudioEntity`).
* ⚠️ **`RwIm3D*`** (effects/lines) still call into the original exe — not bridged yet.
* ⚠️ RxPipeline cosmetics (car env-map / custom building) + ImGui debug UI disabled.
* ⚠️ **Windowed mode**: window is now fixed 800×600 (no `SDL_WINDOW_RESIZABLE`) standalone; RDP test later.

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
> 📋 **See `VERIFY.md`** for the full feature×disassembly verification plan (addresses + status). **Reverse `gta_sa.exe` FIRST**
> (top-priority reference) for every feature.
1. **Text / font (blocker)**: the font texture is `rwFormat=768` (**C4444**, 16-bit, `D3DFMT_A4R4G4B4`) and **doesn't
   sample in librw** (the 32-bit bg does). Fix librw's 16-bit texture handling (or convert the font raster to C8888),
   then implement `CFont::{RenderFontBuffer,ProcessCurrentString}` faithfully (disasm 0x719840 / 0x71A220).
2. **Verify the menu draw path** (`CMenuManager::DrawFrontEnd/DrawBackground/DrawStandardMenus`) vs disasm.
3. Sweep remaining `VERIFY.md` rows (audio, `CDecisionMakerTypes`, `CMemoryMgr`, …) — disasm-first.
4. **Audio** (fix `m_AudioEntity` usage; re-enable `CAESoundManager::Service`).
5. **`RwIm3D*` bridging** (effects/lines) — next big RW task.

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
