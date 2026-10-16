# PROGRESS.md — «Запустить игру» (standalone)

> **Цель этого файла:** вести прогресс к одной конкретной цели — **запустить `gta_reversed.exe` как игру**
> (не мод), шаг за шагом, от boot до игрового цикла. Статусы: ✅ сделано · 🚧 в работе · ⏳ ждёт · ❌ блокер.

## Boot-цепочка standalone (что уже пройдено)

### 2026-10-16: актуальное уточнение измерений

* Обе сборки проходят: `reSA.exe` и `gta_reversed.asi`.
* Загрузка завершается, игрок создаётся; standalone прошёл дальше по player/collision path.
* **`Frames=400` — обороты внешнего цикла, НЕ 400 кадров мира.** Новая метрика
  `completedGameIdleCalls` показала 0 в таком smoke-тесте. Устойчивый игровой цикл ещё не подтверждён.
* Настоящий тест: `tools/probe.ps1 -GameFrames 3 -TimeoutSec 240` → AV на raw thunk
  `CGarage::Update` (0x44AA50), текущий блокер. REA/Ghidra: тело 7553 байта.
* `CPed::ProcessEntityCollision` и `CPlayerInfo::Process` пока явные standalone-заглушки; коллизии педов
  и player-info переходы не работают. Рендер мира и нормальный shutdown не проверены.
* `tools/test_ped_collision_steps.py`: 12 регрессионных случаев арифметики production body проходят.

Ниже сохранена историческая boot-таблица; текущий hand-off — начало `RESUME.md`.

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
| 14 | Старт новой игры / загрузка мира | 🚧 | **проходит**: `CGame::Init1` → ассеты → `CPlantMgr` (DFF через librw) → `CModelInfo` → `g_fx.Init` → **123+ файлов** (все 75 IDE, `MAP.ZON`, `INFO.ZON`, все IPL) → упор в нереверснутые `AddOne`/`GenerateNewOne`-подобные функции (`CPickups`, `Interior_c`, `InitTrains`) и далее `RpAtomic`-рендер |
| 15 | Оконный режим / RDP | 🚧 | окно фиксированное 800×600 (без `SDL_WINDOW_RESIZABLE`) под standalone; RDP-тест — позже |
| 16 | Статические глобалы с C++-конструкторами | ⏳ | в standalone их область обнулена; сконструированы `CHudColours` + 9 сторов моделей. Остальные (39 полиморфных) и **табличные** статики (`VehicleNames` ✅, `ms_aAnimAssocDefinitions` ❌) — см. `IDEAS.md` |
| 17 | Мышь / клик в меню | ✅ | `ForegroundApp` жил только в Win32 `WndProc` (не компилится под SDL) ⇒ был `false` ⇒ `CPad::UpdateMouse()` выходил рано и кнопки не доходили до `NewMouseControllerState`. Теперь ведётся из `SDL_EVENT_WINDOW_FOCUS_*` |
| 18 | Отладка без модальных окон | ✅ | `VERIFY` стал нeфатальным (логирует и продолжает = автоматический «Игнор»), `NOTSA_UNREACHABLE` в standalone выходит с кодом 4 без окна, все CRT-отчёты пишутся в `logs\runtime_reports.txt` (читаемо на лету) |

> **Веха (2026-10-12, вечер #4):** **новая игра ЗАГРУЖАЕТСЯ**. Пройдено: FX-проект (`effects.fxp`), все
> 210 имён машин, 75 IDE-файлов (весь Лос-Сантос/Сан-Фиерро/Вегас/кантри/интерьеры), `MAP.ZON`, `INFO.ZON`
> и **все IPL** (`LAn` → `countryW`). Измерение прогресса — число маркеров `Loading the Game -> Loading`
> в логе: **1 → 44 → 75 → 77 → 123**. Меню полностью работает (текст + мышь + клик).
> Исправлено 3 реальных бага апстрима/наших заголовков (`CMeleeInfo` layout, `m_aComboData` size 12→13,
> `*reinterpret_cast<e2dEffectType*>(type)`), см. `VERIFY.md` §G.

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
