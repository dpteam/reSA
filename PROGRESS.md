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
| 10 | Игровой цикл (`MainLoop`/`ProcessGameLogic`) | ✅ | `GAME_STATE_INITIAL` (splash), `CLoadingScreen`, `CGamma`, `GetMillisecondTime`, `InitialiseEssentialsAfterRW`, `InitialiseCoreDataAfterRW`; дошло до `FRONTEND_IDLE` |
| 11 | Фронтенд: работает, рисует и **навигируется** | ✅ | `FRONTEND_LOADING/LOADED/IDLE`; `FrontendIdle`→`CMenuManager` **реально выводит 2D** (фоновые картинки меню — VINEWOOD/пустыня/палмы и т.п. из frontend TXD); ввод работает (Enter/меню реагирует) |
| 12 | **Вьюпорт / масштаб кадра** | ✅ | раньше всё рисовалось в 640×448 в углу крупного окна; теперь `RsGlobal`/растр камеры = client-размер окна (800×600), картинка на всю ширину. (Как в reVC: `psSelectDevice`, оконная ветка.) |
| 13 | Текст на экране | ✅ | **РЕШЕНО (2026-10-12):** меню читается настоящими шрифтами GTA («Main Menu» готикой, «START GAME/OPTIONS/QUIT GAME», курсор, `TEST / RW 3.6.0.0.3`). `CFont` портирован (PR #1414); невидимость дали **две наши** ошибки: рассинхрон **значений** `ALPHATESTFUNCTION` в мосте (OOB-чтение → альфа-тест резал непрозрачные пиксели) и **ненулевой обнулённый `CHudColours`** (→ `DropColor.a=0` → весь текст с alpha 0). См. `MEMORY.md` §5f, `VERIFY.md` §D |
| 14 | Старт новой игры / загрузка мира | 🚧 | Enter → `CGame::Init1` → загрузка ассетов → `CPlantMgr`/модели; упор в нереверснутое `RpClump`/`RpAtomic` (`RpClumpStreamRead` 0x74B420, не bridge'нуто) |
| 15 | Оконный режим / RDP | 🚧 | окно теперь фиксированное 800×600 (без `SDL_WINDOW_RESIZABLE`) под standalone; RDP-тест — позже |
| 16 | Статические глобалы с C++-конструкторами | ⏳ | в standalone их область просто обнулена; сконструирован пока только `CHudColours`. Остальные кандидаты (`CText`, `CPlayerInfo`, `CCamera`, `CFireManager`, `Fx*_c`, …) — см. `IDEAS.md` |

> **Веха (2026-10-12, вечер #3):** **ТЕКСТ ВИДЕН.** Меню полностьючитаемо настоящими шрифтами GTA, подсветка
> выбранного пункта и курсор на месте, ~38–45 FPS, без крашей. librw-рендер оказался ни при чём — блоккер
> состоял из двух наших багов (перевод render-state **значений**, а не только ID; и необязательная
> инициализация игровых статиков). **Никакого TrueType-обхода не понадобилось.**
> Предыстория/отброшенные гипотезы записаны, чтобы к ним не возвращаться (`IDEAS.md` §«CFont / ТЕКСТ ВИДЕН»).

> **Веха (2026-10-12, ночь #2):** standalone-игра **рисует фронтенд (меню) правильно по размеру** — на экране
> реальные картинки из frontend TXD (VINEWOOD и т.п.), меню **навигируется**, стабильные **~60 FPS**.
> Починен **вьюпорт** (был 640×448 в углу). Осталось: **текст** (меню нечитаемо), собственно **старт игры**, звук.
> Бинарник — **`reSA.exe`**.

> **Веха (2026-10-12, ночь):** standalone-игра **ЗАПУСКАЕТСЯ и РИСУЕТ**. `reSA.exe` стабильно работает
> (без краша), создаёт окно и **выводит 2D-графику фронтенда** (на экране виден логотип/фон **VINEWOOD**).
> Пройдены: инициализация RW, `InitialiseEssentialsAfterRW`/`InitialiseCoreDataAfterRW`, состояния
> `FRONTEND_LOADING`/`LOADED`/`IDLE`, `CMenuManager::Process`/`DrawFrontEnd` (draw-calls реально доходят
> до librw). Бинарник — **`reSA.exe`**. Известные пробелы: **текст не рисуется**, **звук не работает**,
> часть косметики (RxPipeline, RwIm3D) отключена — см. `IDEAS.md`.
> ⚠️ Также: при прогоне из-под отладчика CRT может ловить проверки — их можно **пропускать** («Пропустить»/Ignore);
> критичный для стека баг `C_PcSave::GenerateGameFilename` уже исправлен.

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
