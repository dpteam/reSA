# PROGRESS.md — «Запустить игру» (standalone)

> **Цель этого файла:** вести прогресс к одной конкретной цели — **запустить `gta_reversed.exe` как игру**
> (не мод), шаг за шагом, от boot до игрового цикла. Статусы: ✅ сделано · 🚧 в работе · ⏳ ждёт · ❌ блокер.

## Boot-цепочка standalone (что уже пройдено)

| # | Шаг | Статус | Заметка |
|---|---|---|---|
| 0 | Компиляция exe | ✅ | `bin/Debug/reSA.exe` |
| 1 | `InjectHooksMain()` | ✅ | 330–480 мс |
| 2 | `NOTSA_WinMain` старт | ✅ | «Starting GTA: San Andreas» |
| 3 | `CdStreamOpen MODELS\GTA3.IMG` | ✅ | игровые данные положены |
| 4 | `psGetMemoryFunctions` (0x745510) | ✅ | → CRT-таблица (`CMemoryMgr`) |
| 5 | `RwEngineInit` / `RwEngineOpen` | ✅ | через мост над librw |
| 6 | `PluginAttach` | ✅ | librw `RegisterCorePlugins()` |
| 7 | `psSelectDevice` + **`RsRwInitialize` целиком** | ✅ | движок стартовал, текстурные настройки |
| 8 | `CGame::InitialiseRenderWare` | ✅ | полностью (камера/world/свет, `CFont`, `CHud`, `CPlayerSkin`, `CPostEffects`, TXD/текстуры) |
| 9 | Загрузка ассетов (TXD/Texture) | ✅ | streams + TXD + TexDictionary + Texture + Raster lock — через librw |
| 10 | Игровой цикл (`MainLoop`/`ProcessGameLogic`) | 🚧 | ✅ `GAME_STATE_INITIAL` (splash), `CLoadingScreen::Init`, `CGamma::Init`, `GetMillisecondTime`, `InitialiseEssentialsAfterRW`, `InitialiseCoreDataAfterRW` (SurfaceInfos, decision makers); сейчас `GAME_STATE_FRONTEND_LOADING` → `RwD3D9GetCaps` |
| 11 | Фронтенд (`FRONTEND_LOADED`/`IDLE` → меню) | ⏳ | |
| 12 | Загрузка мира (IMG/IPL/IDE) | ⏳ | |
| 13 | Рендер + оконный режим (RDP) | ⏳ | |

> **Веха (2026-10-12):** standalone **полностью проходит инициализацию RenderWare** и входит в **игровой цикл**
> `MainLoop` / `ProcessGameLogic`: отрабатывают загрузочные экраны (`CLoadingScreen`), таймер, гамма,
> затем `CGame::InitialiseEssentialsAfterRW` **и** `CGame::InitialiseCoreDataAfterRW` (surface-данные, decision makers,
> и т.д.) — вплоть до состояния `GAME_STATE_FRONTEND_LOADING`, где приложение **создаёт окно (1692×919)
> и загружает данные** (`AMERICAN.GXT`, handlings, decision makers).
> Бинарник — **`reSA.exe`**. Debug/tools-UI (ImGui) и RxPipeline-эффекты (env-map, custom building) в standalone
> отключены (см. `IDEAS.md`).

## Этап «RenderWare boot» (текущий)

План: librw вендорнут (`extern/librw`). Реализуем boot-критичные `Rw*` через **мост** (`RwBridge`) —
librw изолирован в отдельном `.cpp`, чтобы не конфликтовали макросы/типы с SA-заголовками.

- [x] librw вендорнут и линкуется (D3D9)
- [x] `RwBridge` (engine): init/open/start/stop/close/term, subsystem, videomode, version, plugin register/offset
- [x] `rwplcore.cpp`: engine-функции под `#ifdef NOTSA_STANDALONE` → мост (PCH отключён для моста в CMake)
- [x] `psGetMemoryFunctions` → CRT-таблица (standalone)
- [x] `RwEngineInit`/`Open`/`Start`, `PluginAttach`, `psSelectDevice`, `RsRwInitialize` — проходят
- [x] Камера/frame/растры/матрицы, world, свет — проходят (через мост)
- [ ] Загрузка ассетов: `RwStreamOpen`/`FindChunk` + `RwTexDictionaryGtaStreamRead` — в работе

### Заметки по реализации
- Мост: `source/game_sa/RenderWare/RwBridge.{h,cpp}` (рядом с `RenderWare.h`). `.cpp` **не** включает `StdInc.h`
  (PCH отключён в CMake). Единственный TU, который включает `<rw.h>` (librw).
- Макросы, читающие SA-офсеты («расходящиеся» поля), переопределяются под `NOTSA_STANDALONE` на вызовы моста:
  `RwCameraGet/SetRaster`, `Get/SetZRaster`, `GetFrame`, `RwRasterGetWidth/Height` (`rwcore.h`),
  `RpLightSetFlags`/`SetFrame` (`rpworld.h`).
- Проверено: конфликтов `rw.h`/`args.h` между SA-заголовками и librw нет.

## Как проверяем
`bin/Debug/reSA.exe` из папки с данными (`bin/Debug`), лог — `logs/log.log` (там стек при падении).
Каждый прогон = насколько далеко прошли. Цель — дойти до окна/меню.
