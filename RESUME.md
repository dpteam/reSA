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

## TL;DR - where we are
**The standalone game `reSA.exe` RUNS: menu with real GTA text + working mouse, and "New Game" now loads the
whole map data (123+ files: all IDE, both ZON, all IPL) instead of dying at the first hurdle.**

* Boots -> `MainLoop`/`ProcessGameLogic` -> `FRONTEND_*` -> `CMenuManager`; renders the real frontend TXD
  backgrounds **and** real GTA fonts ("Main Menu", "START GAME/OPTIONS/QUIT GAME", cursor,
  `TEST / RW 3.6.0.0.3`). **Mouse and clicking work.** ~45-60 FPS. RenderWare is **librw - now OUR FORK**
  (`librw/`, see `librw/FORK.md`), via the C-style bridge (`source/game_sa/RenderWare/RwBridge.*`).
* **New game progresses**: `CGame::Init1` -> grass DFFs parsed by librw -> `CModelInfo` -> `g_fx.Init`
  (`effects.fxp`) -> **75 IDE files** (all of LS/SF/Vegas/countryside/interiors) -> `MAP.ZON`, `INFO.ZON` ->
  **all IPL files**. Yardstick = count of `Loading the Game -> Loading` lines in `logs/log.log`
  (**1 -> 44 -> 75 -> 77 -> 123**).
* Remaining blockers are the ~488 functions still `plugin::Call`ing into the unmapped original exe - mostly
  *runtime* (HUD, PathFind, gangs, tasks), not loading. Ordered list in `VERIFY.md` section G.

### Text/fonts - SOLVED (two of OUR bugs, not librw) - full detail in `MEMORY.md` 5f, `VERIFY.md` D
1. The bridge translated render-state **IDs but not VALUES**: SA's `RwAlphaTestFunction` is D3D-numbered
   (1..8) while librw has 3 values, so `alphafuncMap[5]` read **out of bounds** -> measured `D3DCMP_LESS`
   with ref 2 -> librw's per-`hasAlpha`-texture alpha test **discarded the opaque glyph texels**. Hence
   every `hasAlpha=1` texture was invisible while every opaque one drew.
2. `CHudColours` (a fixed-address static) was never constructed -> zero HUD table -> `DropColor.a=0`, and
   `CFont::PrintString` restores only `r,g,b` after outlines -> **all text alpha 0**.
=> No TrueType fallback was needed; librw's textured im2d path was never broken.

### Debugging infra added this session (keep it) - `VERIFY.md` B2, `IDEAS.md`
* `VERIFY` is **non-fatal** in standalone (logs + continues = an automatic "Ignore"), `NOTSA_UNREACHABLE`
  exits with code **4** and pops no dialog, and every CRT report goes to `logs\runtime_reports.txt`
  (opened `_SH_DENYNO`, so it is tail-able while the game runs). Before this, a suppressed assert was a
  *silent* process death, and `NOTSA_UNREACHABLE` still showed its own modal box - that box is what the
  user had been clicking through.
* **Evidence over experiments**: what cracked the font bug was re-reading an *already-captured* D3D state
  log (`FUNC=0x2 REF=2`) plus a per-string colour log (`#ACCBF1**00**`). A/B render experiments cost days
  and misled. Same here - the non-fatal VERIFY with exact expression + call-site found 3 real bugs in one run.

### Known gaps (details in `IDEAS.md` / `VERIFY.md`)
* **IPL scene objects** - `CPickups::GenerateNewOne` (CObject/blip/message setup), `Interior_c`/
  `InteriorGroup_c`, `InitTrains`; then `CGarage::TidyUpGarage*`/`InitDoorsAtStart` (runtime).
* **`RpAtomic` rendering** - clumps *parse* via librw now, nothing draws them yet
  (`CVisibilityPlugins` render path, `RwIm3D*`).
* **Static globals** - only `CHudColours` + the 9 model stores constructed; 39 candidates remain, plus
  the "tabular statics" class (`VehicleNames` done, `ms_aAnimAssocDefinitions` = 118 baked anim groups -
  the big one).
* **CFont full parity** - outline/shadow alpha, tokens, subtitles (`FONT1`), `FONT_PRICEDOWN`.
* **No sound** - audio `plugin::Call`s stubbed (`CAESoundManager::Service`,
  `CAEStreamingChannel::Service`, `StopRadio`, `ClearMissionAudio`, `UpdateRespectStat`).
* RxPipeline cosmetics (car env-map/dirt, custom buildings) + ImGui debug UI disabled. Window fixed 800x600.

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
> (top-priority reference) for every feature. `VERIFY.md` **section G** is the live new-game/load checklist.
1. **Keep walking the load path** (`VERIFY.md` §G). Yardstick = `Loading the Game -> Loading` marker count
   (currently **123**). The blockers now are the *things the reversed IPL handlers call*:
   `CPickups::GenerateNewOne` remainder (CObject/blip/message), `Interior_c`/`InteriorGroup_c`
   (`AddPickups`/`AddInterior`/`AddGotoPt`/`SetupPeds`…), `InitTrains`, then `CGarage::TidyUpGarage*` /
   `InitDoorsAtStart` (runtime, immediately after load).
2. **Disasm-verify the drafted bodies that were enabled unverified** (§G ⚠️ rows): `FxInfoManager_c::Load`,
   `FindClosestPtOnLine`, `CPickups::GenerateNewOne`, the anim-group fallback, the garage stub.
3. **`RpAtomic` rendering** — clumps parse via librw, nothing draws them (`CVisibilityPlugins`, `RwIm3D*`).
4. **Static globals** — generalize `InitGameGlobalStatics` (39 polymorphic candidates) and audit the
   **"tabular statics"** class; `ms_aAnimAssocDefinitions` (118 baked anim groups) is the biggest known gap.
5. **Verify the menu draw path** (`CMenuManager::DrawFrontEnd/DrawBackground/DrawStandardMenus`) vs disasm,
   and bring **CFont** to full parity (outline/shadow alpha, tokens, subtitles `FONT1`, `FONT_PRICEDOWN`).
6. **Audio** (fix `m_AudioEntity` usage; re-enable `CAESoundManager::Service` / `CAEStreamingChannel::Service`).

## Key references / where to look
* `VERIFY.md` — **section G = the new-game/load checklist** (this is the live work list); §D fonts; §B2 input.
* `PROGRESS.md` — boot-chain status table (rows 13–18 updated).
* `MEMORY.md` §2 (env), §3 (RenderWare/librw), §5a (boot progress + crash list), **§5f (why text was invisible
  + the method lesson)**, §8 (run/dbg), §9–§11 (RW replacement).
* `IDEAS.md` — backlog + all standalone stubs/gaps, the **61 shadowed drafted bodies** find, the
  **std::format**-throws class of bugs, the static-globals list, and the multiplayer plan (§6).
* `refs/reVC` / `refs/re3` — **closest prior art**; when something isn't reversed, check these for a direct/near equivalent
  (`src/audio/`, `src/text/`, `src/renderer/`, `src/fakerw/`). Rule: read-only, reimplement in our own style (see `refs/README.md`).

## Commands cheat-sheet
```powershell
# build (standalone). The wrapper fakes exit 1 - check for the reSA.exe line & no "error C/LNK".
python setup.py --build -o standalone=True -s compiler.version=194

# run + auto-skip CRT dialogs + screenshot + log tail
powershell -ExecutionPolicy Bypass -File .\run_test.ps1 -Seconds 60 -Shot shots\menu.png

# leave it running for manual play (the user drives the menu), then read BOTH logs:
Start-Process .\bin\Debug\reSA.exe -WorkingDirectory .\bin\Debug     # cwd matters (assets live in bin/Debug)
Get-Content .\bin\Debug\logs\log.log -Tail 40                        # normal log + crash stacks
# runtime_reports.txt is held open by the game - read it with share-read, not Get-Content:
$f='.\bin\Debug\logs\runtime_reports.txt'; $s=[IO.File]::Open($f,'Open','Read',[IO.FileShare]::ReadWrite)
$r=(New-Object IO.StreamReader($s)).ReadToEnd(); $s.Close()
```
Exit codes of a standalone run: `4` = a `NOTSA_UNREACHABLE` fired (reason is the last lines of `log.log`);
`0xc0000005` = a real access violation (stack is in `log.log`); anything else = look at `runtime_reports.txt`.
