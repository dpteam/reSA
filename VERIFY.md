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
| `RwRenderStateSet/Get` + `TranslateRenderState` | (lib) | ✅ | SA enum order == reVC `fakerw`; mapping matches reVC `RwRenderStateSet` switch |
| `RwIm2D{RenderPrimitive,Indexed,YLine,Triangle}` | (lib) | ⚠️ | routed via bridge; **indexed path output unverified** (see Font issue) |
| `RwIm2DGetNear/FarScreenZ` | (lib) | ✅ | = device zNear/zFar |
| `RwD3D9GetCaps` | 0x7FAD20 | ✅ | = librw cached `D3DCAPS9` |
| `RwD3D9ChangeVideoMode` | 0x7F8640 | ⚠️ | bridged to `rw::Engine::setVideoMode` |
| `RwD3D9ChangeMultiSamplingLevels` | 0x7F8A90 | ⚠️ | bridged to `SetMultiSamplingLevels` |
| `RwTextureGetRaster` | macro | ✅ | = `tex->raster` (offset 0), matches 0x727B30 |

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

## D. Text / fonts (BLOCKER)
| Feature | Status | Note |
|---|---|---|
| Font TXD load (`MODELS/FONTS.TXD`, `font1`/`font1m`, `font2`/`font2m`) | ⚠️ | textures load (512×512, **rwFormat=768 = C4444**, depth 16); data valid (glyph texels `ffff`, ~21% alpha) |
| **Font texture does not appear on screen** | ❌ | even full-screen opaque quad with font2 shows nothing (bg VINEWOOD *does* show) ⇒ **librw doesn't sample the C4444/D3DFMT_A4R4G4B4 font texture** (or the mask). **Root cause to fix (librw).** |
| Glyph geometry | ✅ | solid bars appear with `TEXTURERASTER=NULL` at correct positions ⇒ geometry/coords fine |

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
| `SurfaceInfos_c::LoadAdhesiveLimits` | 0x55D0E0 | ✅ | SetDir/OpenFile("data\\surface.dat")/LoadLine/sscanf — matches ours |
| `CDecisionMakerTypes::{GetInstance,LoadEventIndices,AddDecisionMaker,RemoveDecisionMaker}` | many | ⚠️ | best-effort; verify mapping/return values |
| `CDecision::{SetDefault,Set}` | 0x600530/0x600570 | ⚠️ | best-effort |
| `C_PcSave::GenerateGameFilename` | 0x6190A0 | ⚠️ | capped `sprintf_s` to MAX_PATH (was ~263 → stack overflow) |
| `CCarFXRenderer`/`CCustomBuildingRenderer::Initialise` | — | 🚫 | RxPipeline stubbed |

## Next actions (ordered)
1. **Fix the C4444 font-texture sampling** in librw (or convert the font raster to C8888 in the bridge). Then implement
   `RenderFontBuffer`/`ProcessCurrentString` faithfully per disasm. → unblocks the (unreadable) menu.
2. Verify the **menu draw path** (`DrawFrontEnd`→`DrawBackground`→`DrawStandardMenus`, EntryExit/cDMA if used) vs disasm.
3. Sweep the remaining ❌/⚠️ rows above, one feature at a time, disasm-first.
