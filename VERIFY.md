# VERIFY.md — feature × disassembly verification plan (standalone)

> **Why:** the *real* `gta_sa.exe` (1.0 US, `refs/gtasa10us/gta_sa.exe`) + `gta_sa.pdb` are the **ground truth**.
> For every standalone feature/fix we ship, verify it against the binary's disassembly (or at least confirm the
> function identity) — this is the **top-priority reference** (above `reVC`/`refs`). Addresses are **1.0-US**;
> our build targets a **Compact** exe, so match by **function identity/logic**, not by blindly trusting addresses.
>
> Disasm command: `objdump -d -M intel --start-address=0xADDR --stop-address=0xADDR2 refs/gtasa10us/gta_sa.exe`
> (`objdump`: `C:\Users\DartPower\scoop\apps\mingw\current\bin\objdump.exe`).

Status legend: ✅ verified against disasm · ⚠️ partially/uncertain · ❌ not verified · 🚫 not done/skipped

## 2026-10-16: player path and probe audit

Oracle: `refs/gtasa10us/gta_sa.exe`, SHA-256
`f01a00ce950fa40ca1ed59df0e789848c6edcf6405456274965885d0929343ac`.
Provider: REA / Ghidra 12.1.4. Function names are matched to the local declarations, not inferred solely
from addresses. Ghidra has generic FUN names and inferred signatures; its pseudocode is not source or
runtime evidence. Source references were consulted read-only, never copied verbatim.

| Item | Native identity | Status and limits |
|---|---|---|
| Group behavior | 0x60D350..0x60D59C | Static branch audit: up ==1200, down >=1200, 16-bit timers; recruit-disabled branch skips recruitment; follow remains allowed for non-recruit targets. Interactive branch behavior not runtime-tested. |
| Collision steps | 0x5E3E90..0x5E3FE5 | REA instructions + constant reads; ceil, support minima 4/2, NPC coefficient 5 and elasticity doubling. offsetof assertions pass in real engine compile; 12 extracted-body tests pass with doubles (not an engine collision test). |
| Collision contacts | 0x5E2530 | Identified only; standalone return-0 stub, NOT a faithful collision implementation. Ghidra body has 4342 bytes across two ranges, span 4345. |
| Player info | 0x56F8D0..0x570AAA | Identified only (4571 bytes); standalone no-op, NOT reversed. |
| Current blocker | CGarage::Update, 0x44AA50..0x44C7D0 | Source thunk identified; Ghidra body 7553 bytes. Actual `-GameFrames 3` run faults at entry. Not ported/stubbed in this batch. |
| Probe measurement | WinMain loop vs completed rsIDLE | Old `-Frames 400` clean exit had 0 completed game Idle calls. New `-GameFrames` waits for actual dispatches; world-frame test is currently FAILING. |
| Bounded exit | post-MainLoop bypass | Deliberate harness behavior, not verified shutdown. Prior post-limit AV read 0xDDDDDDDD; owner unknown. Normal exit keeps original game/RW teardown. |
| Mod compatibility | EXE/ASI build | Both compile; new bodies/hooks standalone-guarded. 12 malformed rwplcore #else directives repaired. ASI gameplay not runtime-tested. |

Final runner checks: `-NoAutostart -Frames 40` returned 0 with zero game calls; `-GameFrames 3` returned 1
with AV at 0x44AA50. A later `-Frames 400` repeat also reached that AV, so smoke outcomes are scheduling-
dependent. Unit harness: 12 collision cases and 5 checks of the production probe-limit predicate pass.

Evidence IDs (full records exported to approved scratch `resa-player-frame-evidence-20261016.json`):

* Group: `ev_0a1b743e751f4a0ce64555e5a903f08750a14370f35831c49989e1e8a7b12acd`.
* Collision steps dossier: `ev_9d6ada14525a205e047f3aed4cf96c42d92c54c06579ac5c9efca0ee9701b763`;
  raw instruction audit: `ev_384504bfb80ef48b0d0f46223e1cbc8af13cf14bbc0470d6d0a7c3128ee5926a`.
* Ceil helper: `ev_1a4d296fe08f0708672f84dce548a34703b75ec30cb1706a044ec1a6bd115496`.
* Contacts: `ev_e47d1f90dd75592585c2f0a443fa7aa346a53025d850b81fefd18d0904f2ecee`.
* Player info: `ev_4d81417af40e36928ccccbf121ceb3dab55418f409a9c98d9a9cbd162c4d4b47`.
* Garage: `ev_e4bb0946eba745877a3be9c905d331536321a88f9726d5002b2cf947934230b4`;
  bounded procedure view: `ev_5c0d1df6522a83aa899eea3729083c80f3f8a6eae57450174f72b9aa2b9ca656`.
  Large dossier exceeded MCP transport budget; view was retrieved from retention, not repeated.
  Garage high-pcode value flow is truncated; no complete semantic audit is claimed.

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
count in `logs/log.log` is the yardstick (1 -> 44 -> 75 -> 77 -> 123 -> 175, and now further still: `CGame::Init2` line 509).

| Feature | Addr | Status | Note |
|---|---|---|---|
| `CGame::Init1` (whole) | 0x5B9030-ish | ✅ | completes: `LoadMeleeData`, `g_fx.Init`, `LoadLevel` and the IPL/scene phase all pass |
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
| `CAnimManager::GetAnimationGroupIdByName` | 0x4D3A30-ish | ✅ | the temporary `ANIM_GROUP_DEFAULT` fallback is **gone**: the 118 baked definitions were already reversed upstream as the unused `ms_aAnimAssocDefinitionsX` and are now wired up via `CAnimManager::SeedBakedAssocDefinitions()` (called from `InitGameGlobalStatics`). Verified against the binary: all 118 group + block names match, same order, zero mismatches |
| IPL/IDE section handlers | 0x5B4530… | ✅ | all 19 `LoadScene` cases + `LoadGarage`/`LoadPickup`/`LoadCullZone`/`LoadCarPathNode`/`LoadEntryExit` are reversed (no `plugin::Call`) — the gaps are in what they *call* |
| `CGame::Init2` progress | — | ⚠️ | reached line **509** (`CStats::Init`): past all IDE/ZON/IPL, streaming, animations, weapons, projectiles, garages, ped groups |
| `RpClumpClone` → `CClumpModelInfo::CreateInstance` | 0x749F70 | ✅ | bridged to librw's `Clump::clone()` (frame tree + atomics + plugin data copy) |
| **RtAnim / AnimBlend interpolator** | 0x4D60E0 area | ❌ | **the one real remaining anim gap**, with two independently measured blockers: `RpHAnimHierarchy::currentAnim` is at 0x20 in SA while librw's `HAnimHierarchy` *ends* at 0x20 (`interpolator` at 0x1C); and SA's `RtAnimInterpolator` holds a `pFrames` **pointer** with the callback fields before `maxInterpKeyFrameSize`, whereas librw's `AnimInterpolator` stores frames **inline** (`getFrames() == this+1`) in a different order. An allocation-model mismatch, not an offset shift. Sites isolated: `RpAnimBlendClumpInitSkinned` (`fd->KeyFrame = nullptr`), `CreateInstance` (skip the 2 interpolator calls), `AtomicRemoveAnimFromSkinCB` (no-op). ⚠️ Consequence: skinned peds do not animate, and `PedIK.cpp:29` + the blend fns in `RpAnimBlend.cpp` deref `KeyFrame` **unguarded** (only `Ped.cpp:3474` guards) |
| `CAnimManager` assoc-definition table | 0x8AA5A8 | ✅ | size is **139** = `ANIM_TOTAL_GROUPS`, not 118 (118 is the *baked* count; `Initialise` sets 118 — disasm `mov ds:0xb4ea28,0x76` — then `ANIMGRP.DAT` appends 21). Verified against the binary: exactly 118 non-zero slots, 118..138 zero and unclaimed (next global at 0x8AC158) |
| `RwFrameForAllChildren` / `ForAllObjects` | 0x7F0DC0 / 0x7F1200 | ✅ | bridged; semantics taken from disasm (callback returns NULL to **stop**; both return the input frame) and `ForAllObjects` latches `next` before the callback as the original does. The whole `RwFrame*` API is now bridged |
| `RpSkin*` plugin family | 0x7C7550… | ✅ | bridged to librw's `Skin`/`HAnimHierarchy`. `RpSkin` is opaque in SA so substituting is safe; field map verified (numBones 0x00, inverseMatrices 0x0C, indices 0x14, weights 0x18). `RpSkinAtomic{Set,Get}Type` deliberately unbridged (queries device caps; only caller is runtime clothing) |
| `CWaterLevel::FillQuadsAndTrianglesList` | 0x6E7B30 | 🚫 | stubbed: outputs are read only by `PreRenderWater`/`RenderWater` (both unbridged) ⇒ no visible water until then |
| `CProjectileInfo::Initialise` | 0x737B40 | ✅ | reversed (58 bytes, pure static init). The original writes 16 (=`WEAPON_GRENADE`) into **every** `m_nWeaponType` (ecx never decremented) and does **not** clear `m_pVictim`/`m_vecLastPosn` — reproduced exactly, not "tidied" |
| `CGarage::InitDoorsAtStart` | 0x447600 | ✅ | reversed: `flags = (flags & 0x39) \| 0x40`, door CLOSED, `timeToOpen` 0, then switch on **`m_nType`** (0x4C; jump table indexed by type-1, read out of the binary): types 2..5 (bomb shops, pay 'spray) start OPEN with `doorPosition` 1.0; types 1, 6..12, 14..45 get `doorPosition` 0; types 0 and 13 unchanged |
| `CPedGroups::Init` | 0x5FB8C0 | ⚠️ | only the reachable branch implemented (`ScriptReferenceIndex.fill(1)`). The active-group branch needs 0x5F80D0/0x5F7350 (undeclared here), but `ms_activeGroups` is provably all-zero at `CTheScripts::Init` (sole writer is `AddGroup`, runtime) — guarded by a `VERIFY` so it fails loudly if that stops holding |
| `CStats::{CheckForStatsMessage,PopulateFavoriteRadioStationList,LoadStatUpdateConditions}` | 0x559760 / 0x558EC0 / 0x559860 | 🚫 | stubbed as a group — all HUD message plumbing: `STATDISP.DAT`'s own header says "selects when to show messages based on stat values", and `CheckForStatsMessage` only walks a 128-entry display table raising HUD messages. Reverse together |
| `CAttractorScanner::Clear` | 0x5FFF90 | ✅ | reversed (62 bytes) + header corrected: `field_68` is **float** (squared distances 225.0 / 25.0), not int32; nothing else in the tree reads these fields |
| `Get2DEffectAtomicCallback` | 0x734850 | ✅ | reversed (34 bytes) via `RpGeometryGet2dFxCount` instead of poking offsets |
| `RwD3D9{Set,Get}StencilClear` | 0x7F9D30 / 0x7F9D40 | ✅ | 2-instruction driver-global setter (disasm); bridge stores it (librw has no slot, nothing reads it back) |
| `CMissionCleanup::CheckIfCollisionHasLoadedForMissionObjects` | 0x4652D0 | 🚫 | stubbed, provably inert (list all-EMPTY at this stage; only writer is `AddEntityToList`). Deps still unreversed: 0x4633E0, 0x542800, model-info via 0xB7449C |
| **Script VM / opcode dispatch** | table 0x8A6168 | 🟡 | **no longer the blocker.** The 27 original dispatchers are still absent, but arity is now EXTRACTED from their case bodies (`tools/opcode_arity.py` -> `Scripts/OpcodeArity.gen.inc`, 27/27 dispatchers, opcodes 0..2638: 1118 known-operand-count, 1015 read-nothing, 394 irregular), so un-hooked opcodes are skipped exactly instead of killing their script. A run now reports **zero** skipped opcodes on the startup path. Implemented from disasm since: 0x42C, 0x30D, 0x997, 0x998, 0x111, 0x4E4. Remaining: the 394 irregular opcodes still terminate their script if hit, and the ~2088 un-hooked ones do nothing (correctly, but silently) |
| `CRunningScript::ShutdownThisScript` | 0x465AA0 | ❌ | unreversed; its drafted body needs `GetStreamedScriptWithThisStartAddress` (not declared in this tree). Skipping it leaks the streamed-script status refcount + ped-brain cleanup (documented inline) |
| `CGame::Initialise` | — | ✅ | **completes**, and the per-frame loop runs: `--autostart` probe reports `reachedGameLoop=1`. Current wall is one specific thing: the player model's RW object is uninitialised memory (`GetRpClump` sees type 16/32/48/216 across runs - a value that VARIES is the proof it is uninitialised, not a foreign struct), so `CPed::SetModelIndex` -> `UpdateRpHAnim` calls into 0xCDCDCDCD. `MODELS\PLAYER.IMG` does open, so this is model-info population, not archive access |
| Rw matrix/vector family (`RwMatrixUpdate`, Multiply, Invert, Transform, Rotate, Translate, Scale, `RwV3dTransform*`) | 0x7F18A0… | ✅ | bridged. `RwMatrixUpdate` is 2 instructions and equals librw's `Matrix::update()`; operand order and `CombineOp` (PRE=0/POST=1) checked against librw |
| `_rwStreamInitialize` (memory streams) | 0x7EC810 | ✅ | bridged to **one cached `rw::StreamMemory`, re-opened per call** (the original re-initialises the single `gRwStream` buffer in place). `StreamClose` must NOT delete it — the game closes the same stream twice (`Streaming.cpp:476` and `:499`) |

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
