# VERIFY.md — feature × disassembly verification plan (standalone)

> **Why:** the *real* `gta_sa.exe` (1.0 US, `refs/gtasa10us/gta_sa.exe`) + `gta_sa.pdb` are the **ground truth**.
> For every standalone feature/fix we ship, verify it against the binary's disassembly (or at least confirm the
> function identity) — this is the **top-priority reference** (above `reVC`/`refs`). Addresses are **1.0-US**;
> our build targets a **Compact** exe, so match by **function identity/logic**, not by blindly trusting addresses.
>
> Disasm command: `objdump -d -M intel --start-address=0xADDR --stop-address=0xADDR2 refs/gtasa10us/gta_sa.exe`
> (`objdump`: `C:\Users\DartPower\scoop\apps\mingw\current\bin\objdump.exe`).

Status legend: ✅ verified against disasm · ⚠️ partially/uncertain · ❌ not verified · 🚫 not done/skipped

## A. Boot / core
| Feature | Addr | Status | Note |
|---|---|---|---|
| `GetMillisecondTime` | 0x5617C0 | ✅ | `call RsTimer` + zero-extend ⇒ our `OS_TimeMS()` exact |
| `CGamma::Init` | 0x747180 | ✅ | sets `{1, 0.5f, -1000, 1000}`; `RwD3D9GetCaps`→`DeclTypes&=~0x80`; caps2 bit 0x20000/0x100000 ⇒ disable |
| `CMemoryMgr::{Malloc,Free,Realloc,Calloc,MallocAlign,FreeAlign}` | — | ❌ | CRT under standalone; verify pool semantics vs `MEMORY_MGR_USE_MEMORY_HEAP` |
| `psGetMemoryFunctions` | 0x745510 | ❌ | our CRT table — verify the struct layout the game expects |
| `psSelectDevice` | 0x746190 | ⚠️ | standalone windowed branch = client rect (matches reVC); verify vs disasm |
| `Cmd`/`InjectHooksMain` (NullHook) | — | ❌ | standalone-only path |

## B. RenderWare (via librw bridge)
| Feature | Addr | Status | Note |
|---|---|---|---|
| `RwRenderStateSet/Get` + `TranslateRenderState` | (lib) | ✅ | SA enum order == reVC `fakerw`; **IDs** verified vs disasm (`0x1D`/`0x1E` etc.). ⚠️ Now also translates **values** for the one enum that differs (`ALPHATESTFUNCTION`, SA 1..8 vs librw 0..2) — see §D |
| `DefinedState`/`DefinedState2d` (render-state defaults) | 0x734650/0x734750 | ✅ | disasm `push 5;push 1Dh` + `push 2;push 1Eh` == our `app.cpp:192/210` (GREATER, ref 2) |
| `CHudColours::CHudColours` (global statics ctor) | 0x58FDA0 | ✅ | 15 `SetRGBAValue` calls incl. `BLACK=(0,0,0,255)`; standalone must call it (`InitGameGlobalStatics`) |
| `RwIm2D{RenderPrimitive,Indexed,YLine,Triangle}` | (lib) | ⚠️ | routed via bridge; **indexed path output unverified** (see Font issue) |
| `RwIm2DGetNear/FarScreenZ` | (lib) | ✅ | = device zNear/zFar |
| `RwD3D9GetCaps` | 0x7FAD20 | ✅ | = librw cached `D3DCAPS9` |
| `RwD3D9ChangeVideoMode` | 0x7F8640 | ⚠️ | bridged to `rw::Engine::setVideoMode` |
| `RwD3D9ChangeMultiSamplingLevels` | 0x7F8A90 | ⚠️ | bridged to `SetMultiSamplingLevels` |
| `RwTextureGetRaster` | macro | ✅ | = `tex->raster` (offset 0), matches 0x727B30 |
| `Rw*/Rp*RegisterPlugin[Stream]`, `GetPluginOffset`, `SetStream{Always,Rights}CallBack` | 0x7F3BB0… | ✅ | bridged to librw's `PluginList` (RW-compatible: `registerPlugin` returns the in-object byte offset, `RWPLUGINOFFSET` means the same on both sides). 45 thunks; `RpWorldSector` deliberately absent (no librw counterpart) |
| `PluginAttach` (game's own RW plugins) | 0x53D870 | ⚠️ | standalone now attaches NodeName/Visibility/TxdStore/Breakable/Collision/2dEffect/Pipeline; **not yet**: RpAnimBlend (needs an RtAnim interpolator), CustomBuilding/CarFX (RxPipeline) |
| `RpClumpStreamRead` + `Rp{Clump,Atomic,Geometry,Material}` accessors | 0x74B420 | ⚠️ | bridged to librw's `Clump/Atomic/Geometry/Material::streamRead`; verified only as far as *parsing* grass DFFs (no render yet) |

## B2. Input (standalone-only)
| Feature | Addr | Status | Note |
|---|---|---|---|
| `ForegroundApp` maintenance | 0x8D621C | ✅ | was only ever written by the Win32 `WndProc` (`WM_ACTIVATEAPP`/`WM_SETFOCUS`/`WM_KILLFOCUS`), which is `#ifndef NOTSA_USE_SDL3`. Zero-filled ⇒ `false` ⇒ `CPad::UpdateMouse()` bailed at `if (!IsForegroundApp()) return;` ⇒ **mouse buttons never reached `NewMouseControllerState`** (keyboard survived because the key exchange happens in `UpdatePads`, outside that guard). Now driven from `SDL_EVENT_WINDOW_FOCUS_{GAINED,LOST}` mirroring the original's semantics (flag + `rsACTIVATE` + `Pad::Clear`), seeded `true` at window creation |
| `CPad::ProcessMouseEvent` / `ProcessKeyboardEvent` | — | ✅ | already correct under SDL; only the foreground gate above was broken |

## C. Frontend / menu (current focus)
| Feature | Addr | Status | Note |
|---|---|---|---|
| `CFont::Initialise` | 0x5BA690 | ✅ | Sprite[0/1]@0xC71AD0/D4, `SetTexture("font2","font2m")` = ours |
| `CFont::PrintChar` | 0x718A10 | ✅ | ported upstream PR #1414 (full CFont) |
| `CFont::RenderFontBuffer` | 0x719840 | ✅ | ported upstream PR #1414 (full reversal) |
| `CFont::ProcessCurrentString` | 0x71A220 | ✅ | ported upstream PR #1414 |
| `CFont` overall | — | ✅ | **upstream PR #1414 "CFont done" (branch `fonts`, `82f34b0`) ported by hand** — full CFont (see Font.cpp/Font.h) |
| `CSprite2d::AddToBuffer` | 0x728200 | ✅ | disasm matched exactly (0x1C stride, buffers 0xC5F958/0xC4B958, thresholds 1532/4090) |
| `CSprite2d::RenderVertexBuffer` | 0x7273D0 | ✅ | disasm matched (FILTER=LINEAR + `RwIm2DRenderIndexedPrimitive(TRILIST,…)`) |
| `CSprite2d::SetVertices` (RwIm2DVertex) | 0x727A00 | ✅ | field order/offsets match ours |
| `CSprite2d::SetRenderState` | 0x727B30 | ✅ | = set TEXTURERASTER to `tex->raster` |
| `CMenuManager::Initialise` | 0x5744D0 | ⚠️ | crash @MSAA/videomode fixed; verify rest |
| `CMenuManager::DrawFrontEnd/DrawBackground/DrawStandardMenus` | — | ❌ | verify against disasm |
| **EntryExit/`cDMA`/draw-list (menu sprite enqueue)** | ? | ❌ | **`CMenuManager` uses `cDMA`/EntryExit queue?** — unknown; may explain text path |
| `CMenuManager::CheckForMenuClosing` | 0x57Bxxx | ❌ | verify |
| `CMenuManager::DrawStandardMenus` / `MENU_ACTION_RESOLUTION` (resolution line) | 0x5794A0 / 0x57A05A | ✅ | disasm: `call GetVideoModeList` → `mov eax,[eax+edx*4]` → `call AsciiToGxtChar`(0x718600) — **no null check**. Upstream replaced that with `std::format("{}", …)`, which *throws* `std::format_error` on a null `char*` ⇒ crashed entering Advanced → Video Settings. Null entries are normal (index 0 is the *windowed* desktop mode, `flags==0`, filtered out by `GetVideoModeList`), and the menu's own navigation already tolerates them (`while(!videoModes[i])`, Process.cpp:508/510). Now guarded + falls back to the real window size |

## D. Text / fonts — ✅ RESOLVED (2026-10-12)
| Feature | Addr | Status | Note |
|---|---|---|---|
| Font TXD load (`font2`/`font2m`, `font1`/`font1m`) | 0x5BA690 | ✅ | 512×512, `rwFormat=0x300`, native **DXT3**; alpha ~17%/22% non-zero (white glyphs) — data OK |
| **Why text was invisible** | — | ✅ | **NOT a librw/texture bug.** Two of our own bugs: |
| ① alpha-test **enum value** desync (SA `RwAlphaTestFunction` 1..8 vs librw `AlphaTestFunc` 0..2) | 0x734750 | ✅ | disasm: `push 5; push 1Dh` (GREATER) + `push 2; push 1Eh` (ref) ⇒ OOB `alphafuncMap[5]` ⇒ `D3DCMP_LESS` ⇒ opaque texels discarded. Fixed by `Translate/UntranslateAlphaTestFunc` + librw clamp |
| ② `CHudColours` global never constructed in standalone | 0x58FDA0 | ✅ | zeroed table ⇒ `DropColor.a=0`; `PrintString` restores only `r,g,b` ⇒ **all text alpha=0** (log: `#ACCBF1**00**`). Fixed by `InitGameGlobalStatics()` |
| Result | — | ✅ | Real GTA fonts render: gothic “Main Menu”, “START GAME/OPTIONS/QUIT GAME”, cursor, `TEST / RW 3.6.0.0.3` |
| Remaining | — | ⏳ | parity sweep of `PrintString`/`ProcessCurrentString`/outline-shadow alpha vs disasm; and the **general static-global** issue (see `IDEAS.md`) |

## E. Audio
| Feature | Addr | Status | Note |
|---|---|---|---|
| `CAESoundManager::Service` | 0x4F0000 | 🚫 | skipped (garbage `m_AudioEntity` crash) — verify audio entity layout |
| `CAERadioTrackManager::Service` | 0x4EB9A0 | 🚫 | skipped |
| `CAEFrontendAudioEntity::{AddAudioEvent,UpdateParameters}` | 0x4DD4A0/0x4DEDA0 | ⚠️ | use reversed bodies; verify |
| `CAEWMADecoder::WMCreateSyncReader` | 0xb6bad8 | ✅ | `StaticRef` (was absolute addr) |

## F. Gameplay / data
| Feature | Addr | Status | Note |
|---|---|---|---|
| `CTaskSimpleFight::LoadMeleeData` | 0x5BEDC0 | ⚠️ | defaults for `m_aComboData` transcribed from disasm 0x5BEDC0..0x5BEE38 (verified). **Not ported**: the 7-entry float table at 0xC177D0 and the `data\melee.dat` parse (0x538900/0x536F80/0x8214D0) that overrides them |
| `CMeleeInfo` layout | — | ✅ | corrected against the same disasm: 0x48/0x4C are **floats** (`m_fBlockHit`/`m_fBlockChain` = 100.0f), 0x50/0x55 are two `uint8[5]` arrays. `VALIDATE_SIZE(…,0x88)` holds |
| `m_aComboData` size | — | ✅ | **13** entries, not 12 — outer loop is `(0xC177BC-0xC170D4)/0x88 = 13`, and `TaskSimpleUseGun.cpp` already read `[12]` (was out of bounds) |
| `FxPrimBP_c::Load` | 0x5C2010 | ⚠️ | **unverified drafted body** enabled for standalone (was shadowed by `plugin::Call`). Needs a disasm pass |
| `FxInfoManager_c::Load` | 0x5C0B70 | ⚠️ | same — drafted body enabled, needs disasm |
| `FindClosestPtOnLine` | 0x4A4880 | ⚠️ | same — drafted body enabled, needs disasm |
| `SurfaceInfos_c::LoadAdhesiveLimits` | 0x55D0E0 | ✅ | SetDir/OpenFile("data\\surface.dat")/LoadLine/sscanf — matches ours |
| `CDecisionMakerTypes::{GetInstance,LoadEventIndices,AddDecisionMaker,RemoveDecisionMaker}` | many | ⚠️ | best-effort; verify mapping/return values |
| `CDecision::{SetDefault,Set}` | 0x600530/0x600570 | ⚠️ | best-effort |
| `C_PcSave::GenerateGameFilename` | 0x6190A0 | ⚠️ | capped `sprintf_s` to MAX_PATH (was ~263 → stack overflow) |
| `CCarFXRenderer`/`CCustomBuildingRenderer::Initialise` | — | 🚫 | RxPipeline stubbed |

## G. New-game / world load (current focus, 2026-10-12)
Progress is measured by how far `CGame::Init1/Init2` → `CFileLoader::LoadLevel` gets; the load marker
count in `logs/log.log` is the yardstick (1 → 44 → 75 → 77 → **123** IDE/ZON/IPL files so far).

| Feature | Addr | Status | Note |
|---|---|---|---|
| `CGame::Init1` (whole) | 0x5B9030-ish | ⚠️ | reaches `LoadLevel`; `LoadMeleeData` + `g_fx.Init` done, rest of `Init2/Init3` not reached yet |
| `CTaskSimpleFight::LoadMeleeData` | 0x5BEDC0 | ⚠️ | defaults for `m_aComboData` transcribed & verified. **Not ported**: the 7-entry float table at 0xC177D0 and the `data\melee.dat` parse that overrides them |
| `CMeleeInfo` layout / `m_aComboData` size | — | ✅ | corrected from disasm: 0x48/0x4C are floats, 0x50/0x55 two `uint8[5]`; the array is **13** entries (was 12 — `TaskSimpleUseGun.cpp` already read `[12]`, i.e. out of bounds) |
| `FxPrimBP_c::Load` | 0x5C2010 | ✅ | drafted body enabled; **header line must be read as bare `%s`** (`ReadField<void>`), not `%s %s` — verified against disasm (formats at 0x85C714/0x869860/0x86B2C4) and against `models/effects.fxp`. The rest of the body matches the disasm |
| `FxInfoManager_c::Load` / `FindClosestPtOnLine` | 0x5C0B70 / 0x4A4880 | ⚠️ | drafted bodies enabled; **not yet disasm-verified** |
| `CModelInfo::Initialise` + the 9 model stores | 0x4C6810 | ✅ | `mi.Init()` is *virtual* (0x4C6620 `call [eax+0x18]`) so the zero-filled stores faulted; now constructed via `notsa::ConstructStatic` |
| `cHandlingDataMgr::GetHandlingId` / `VehicleNames` | 0x6F4FD0 / 0x8D3978 | ✅ | 210 baked names are zero-filled in standalone ⇒ `FindExactWord` matched slot 0 for everything (`strlen("")==0`) and `GetHandlingId` found nothing. Now interned while parsing; order verified against the original exe |
| `CFileLoader::Load2dEffect` | 0x5B7670 | ✅ | drafted body enabled + **upstream typo fixed**: `*reinterpret_cast<e2dEffectType*>(type)` dereferenced the *value* (type=5 ⇒ AV at 5); disasm 0x5B76FA is `mov al,[esp+0xC]; mov [esi+0xC],al`. Per-type switch still upstream's empty `todo:` |
| `CModelInfo::Get2dEffectStore` | 0x4C5A60 | ✅ | disasm is `mov eax,0xB4C2D8; ret` ⇒ literally `&ms_2dFXInfoStore` |
| `CModelInfo::GetModelInfo` | 0x4C6560-ish | ✅ | the original reads **past** its 20000-entry table on purpose (`RequestModelStream` fetches before narrowing to DFF, and `COLToModelId(1)==25001`); harmless there, fatal with our exactly-sized static storage ⇒ clamped to nullptr in standalone |
| `RwMatrixUpdate` | 0x7F18A0 | ✅ | disasm is 2 instructions (`and dword [eax+0xC],0xFFFDFFFC`); librw's `Matrix::update()` is identical and the layout matches ⇒ no bridge needed |
| `RwMatrix*` / `RwV3dTransform*` family | 0x7F18B0… | ✅ | bridged (Create/Destroy/Multiply/Invert/Transpose/Transform/Rotate/Translate/Scale/SetIdentity/Optimize + V3d points/vectors); operand order & `CombineOp` checked against librw |
| `_rwStreamInitialize` | 0x7EC810 | ✅ | bridges to **one cached `rw::StreamMemory`, re-opened per call** (the original re-initialises the single `gRwStream` buffer in place). `StreamClose` must NOT delete it — the game closes it twice (Streaming.cpp:476 and :499) |
| `CCarFXRenderer::InitialiseDirtTexture` | 0x5D5BC0 | 🚫 | stubbed: loads 16 dirt textures consumed only behind `if (m_apDirtMaterials[i])`, which only the (unavailable) RxPipeline renderer ever fills |
| `CGarages::AddOne` | 0x4471E0 | 🚫 | stubbed (~12 KB unreversed): fills the `aGarages[NumGarages]` record from the IPL params with obvious defaults + bounds guard. `TidyUpGarage*`/`InitDoorsAtStart` are runtime-only, so not on this path |
| `CPickups::GenerateNewOne` | 0x456F20 | ⚠️ | implemented the slot claim + field fill (free == `PICKUP_NONE`, ref via `GetNewUniquePickupIndex`); the original's CObject / blip / message / type-specific setup is TODO |
| `CAnimManager::GetAnimationGroupIdByName` | 0x4D3A30-ish | ⚠️ | falls back to `ANIM_GROUP_DEFAULT` + one WARN. The 118 baked `ms_aAnimAssocDefinitions` are **not** reconstructed (big job — see IDEAS.md) |
| IPL/IDE section handlers | 0x5B4530… | ✅ | all 19 `LoadScene` cases + `LoadGarage`/`LoadPickup`/`LoadCullZone`/`LoadCarPathNode`/`LoadEntryExit` are reversed (no `plugin::Call`) — the gaps are in what they *call* |

**Scale note:** 488 functions in the tree still `plugin::Call` into the original exe. Most are runtime
(HUD draw, PathFind routing, gangs, tasks), not loading. Prioritise by what the load path actually reaches.

## Next actions (ordered)
1. ~~Fix the font texture~~ ✅ **done** — see §D. Text is visible with the real GTA fonts.
2. ~~Get the new game past `CGame::Init1`~~ ✅ largely — see §G. The load walks 123+ files and reaches IPL
   scene loading (`LAn.IPL` → `countryW.IPL`); the remaining blockers are in what the section handlers call.
3. **Keep walking the load path** (§G): of the 488 remaining `plugin::Call` functions, only the ones the
   loader reaches matter now — `CPickups` (CObject/blip/message setup), `Interior_c`/`InteriorGroup_c`
   (`AddPickups`/`AddInterior`/`SetupPeds`…), `InitTrains`, then `CGarage::TidyUpGarage*`/`InitDoorsAtStart`
   (runtime, immediately after load). Enable/verify one at a time.
4. **Disasm-verify the drafted bodies enabled unverified** (§G ⚠️ rows): `FxInfoManager_c::Load`,
   `FindClosestPtOnLine`, `CPickups::GenerateNewOne`, the anim-group fallback, the garage stub.
5. **Generalize `InitGameGlobalStatics`** — one instance of *every* fixed-address global with a C++ ctor
   being zero-filled. Scan: 2352 `StaticRef` sites / 641 types, **39 polymorphic-or-ctor** + the 10 `CStore<>`
   model stores; done so far: `CHudColours` + 9 stores. Plus the **"tabular statics"** class (`VehicleNames`
   ✅, `ms_aAnimAssocDefinitions` ❌) — audit other `StaticRef<T[N]>` / `StaticRef<char[N][M]>`.
6. Bring **CFont to full disasm parity** (outline/shadow alpha, tokens, subtitles `FONT1`, `FONT_PRICEDOWN`).
7. Verify the **menu draw path** and sweep the remaining ❌/⚠️ rows, one feature at a time, disasm-first.

