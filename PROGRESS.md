# PROGRESS.md — «Запустить игру» (standalone)

> **Цель этого файла:** вести прогресс к одной конкретной цели — **запустить `gta_reversed.exe` как игру**
> (не мод), шаг за шагом, от boot до игрового цикла. Статусы: ✅ сделано · 🚧 в работе · ⏳ ждёт · ❌ блокер.

## Boot-цепочка standalone (что уже пройдено)

| # | Шаг | Статус | Заметка |
|---|---|---|---|
| 0 | Компиляция exe | ✅ | `bin/Debug/gta_reversed.exe` |
| 1 | `InjectHooksMain()` | ✅ | 330–480 мс |
| 2 | `NOTSA_WinMain` старт | ✅ | «Starting GTA: San Andreas» |
| 3 | `CdStreamOpen MODELS\GTA3.IMG` | ✅ | игровые данные положены |
| 4 | `psGetMemoryFunctions` (0x745510) | ✅ | → CRT-таблица (`CMemoryMgr`) |
| 5 | `RwEngineInit` / `RwEngineOpen` | ✅ | через мост над librw |
| 6 | `PluginAttach` | ✅ | librw `RegisterCorePlugins()` |
| 7 | `psSelectDevice` + **`RsRwInitialize` целиком** | ✅ | движок стартовал, текстурные настройки |
| 8 | `CGame::InitialiseRenderWare` | 🚧 | камера/frame/растры/матрицы ✅, world+свет ✅, `CreateDebugFont` ✅, `CFont::Initialise`: TXD загружен ✅ → сейчас `RwTextureRead` (текстуры шрифта) |
| 9 | Загрузка ассетов (TXD/DFF/стриминг) | 🚧 | streams + TXD + TexDictionary текущий + Texture (read/create/add) — через librw |
| 10 | Загрузка мира (IMG/IPL/IDE) | ⏳ | |
| 11 | Рендер + оконный режим (RDP) | ⏳ | |

> **Веха:** движок RenderWare в standalone **работает** (через librw). Создание камеры, мира, света,
> растров и матриц — проходит. Дошли до **загрузки игровых ассетов** (первый реальный TXD).

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
`bin/Debug/gta_reversed.exe` из папки с данными (`bin/Debug`), лог — `logs/log.log` (там стек при падении).
Каждый прогон = насколько далеко прошли. Цель — дойти до окна/меню.
