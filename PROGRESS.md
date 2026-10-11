# PROGRESS.md — «Запустить игру» (standalone)

> **Цель этого файла:** вести прогресс к одной конкретной цели — **запустить `gta_reversed.exe` как игру**
> (не мод), шаг за шагом, от boot до игрового цикла. Статусы: ✅ сделано · 🚧 в работе · ⏳ ждёт · ❌ блокер.

## Boot-цепочка standalone (что уже пройдено)

| # | Шаг | Статус | Заметка |
|---|---|---|---|
| 0 | Компиляция exe | ✅ | `bin/Debug/gta_reversed.exe` |
| 1 | `InjectHooksMain()` | ✅ | 330–480 мс |
| 2 | `NOTSA_WinMain` старт | ✅ | «Starting GTA: San Andreas» |
| 3 | `CdStreamOpen MODELS\GTA3.IMG` | ✅ | нужны игровые данные (положены) |
| 4 | `psGetMemoryFunctions` (0x745510) | ✅ | → CRT-таблица (`CMemoryMgr`) |
| 5 | `RwEngineInit` / `RwEngineOpen` | ✅ | через мост над librw |
| 6 | `AppEventHandler(rsPLUGINATTACH)` → `PluginAttach` | ✅ | librw `RegisterCorePlugins()` |
| 7 | `psSelectDevice` (subsystem+видеорежим+MSAA) | 🚧 | проходит выбор; краш на `RwD3D9EngineSetMultiSamplingLevels` |
| 8 | `RwEngineStart` + device/videomode | ⏳ | |
| 9 | Загрузка мира/стриминга | ⏳ | |
| 10 | Рендер + оконный режим (RDP) | ⏳ | |

## Этап «RenderWare boot» (текущий)

План: librw вендорнут (`extern/librw`). Реализуем boot-критичные `Rw*` через **мост** (`RwBridge`) —
librw изолирован в отдельном `.cpp`, чтобы не конфликтовали макросы/типы с SA-заголовками.

- [x] librw вендорнут и линкуется (D3D9)
- [x] `RwBridge` (engine): init/open/start/stop/close/term, subsystem, videomode, version, plugin register/offset
- [x] `rwplcore.cpp`: engine-функции под `#ifdef NOTSA_STANDALONE` → мост (PCH отключён для моста в CMake)
- [x] `psGetMemoryFunctions` → CRT-таблица (standalone)
- [x] **`RwEngineInit` + `RwEngineOpen` проходят** (проверено прогоном)
- [ ] Плагины: `PluginAttach` → `RpWorldPluginAttach` (следующий блокер)

### Заметки по реализации
- Мост: `source/game_sa/RenderWare/librw/RwBridge.{h,cpp}`. `.cpp` **не** включает `StdInc.h` (PCH off).
- `libw::librw` даёт нашим TU include-путь librw и `RW_D3D9` (PUBLIC), так что мост компилится.
- Проверено: конфликтов `rw.h`/`args.h` между SA и librw нет.

## Как проверяем
`bin/Debug/gta_reversed.exe` из папки с данными (`bin/Debug`), лог — `logs/log.log` (там стек при падении).
Каждый прогон = насколько далеко прошли. Цель — дойти до окна/меню.
