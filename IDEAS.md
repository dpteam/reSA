# IDEAS.md — backlog & raw ideas

> **Что это:** сырой список идей/задач на будущее. Здесь **ничего не обязано быть реализовано** и ничего
> отсюда не является «планом». Когда что-то отсюда берётся в работу — оно переезжает в `PROGRESS.md`
> (или в трекер задачи), а сюда добавляется пометка «→ взято в работу».
>
> Статусы: 💡 идея · 🔍 проверено · ✅ сделано · 🚫 отклонено

---

## 1. 🎮 Фичи, которые хочется реализовать (general)

- 💡 **Переименовать standalone-бинарник в `reSA.exe`** (сейчас `gta_reversed.exe`). Мод (`.asi`) остаётся
  `gta_reversed.asi`. *(Уже сделано: `OUTPUT_NAME reSA` под standalone.)*
- 💡 **Иконка окна/exe** — взята из игровых данных (`data/Icons/app.ico`), встроена как ресурс (`app.rc`). ✅
- 💡 **Вернуть debug/tools-UI (ImGui) в standalone.** Сейчас он отключён под `NOTSA_STANDALONE`
  (`UIRenderer` + `DebugModules` — no-op), т.к. инициализация ImGui падала до отрисовки. Вернуть **когда-нибудь
  потом** (для отладки удобно), желательно **без ломки** мод-сборки. Вероятные причины падения: устройство D3D9
  создаётся librw и `GetD3D9Device()` мог вернуть не тот указатель / быть null на момент `ImGui_ImplDX9_Init`;
  и для `ImGui_ImplWin32_Init(PSGLOBAL(window))` окно должно существовать. Разобраться отдельной задачей.
- 💡 Оконный режим по умолчанию + корректная работа по RDP — сейчас в standalone оконный режим не
  форсируется (см. `MEMORY.md` §8). Нужно для тестирования по удалённому рабочему столу.
- 💡 **`gta-reversed.ini` в standalone** — проверить, что все опции fast-loader реально применяются.
- 💡 **Логи в отдельный файл + понятная диагностика старта** — уже есть `logs/log.log` с дампом стека; можно
  сделать «boot report» (какие RW-функции решились на librw, какие ещё заглушки).
- 💡 **Автотест «докуда доходит boot»** — маленький скрипт, который запускает exe, парсит `logs/log.log` и
  печатает прогресс (удобно для регрессий).
- 💡 **Пресет CMake/Conan для standalone под VS 2022 (compiler.version=194)** — чтобы не помнить про ключ.
- 💡 **CI-джоба для standalone**, чтобы этот путь не гнил (сейчас CI собирает только `dump_hooks_only`).

## 2. 📥 Портирование изменений из upstream PR (политика)

- 🔍 **Источник:** <https://github.com/gta-reversed/gta-reversed/pulls> (апстрим).
- 💡 **Правило (по просьбе пользователя): НЕ вливать через git** (никаких merge/rebase чужих веток) —
  **портировать код вручную**: скопировать/адаптировать нужные изменения в наш файл, соблюдая наш стиль
  и `#ifdef NOTSA_STANDALONE`-разделение.
- 💡 Пользователь **может помочь скачать нужные форки/ветки/коммиты** — если нужен конкретный PR,
  просто попросить (указать № PR или ссылку).
- 💡 Полезные PR, которые стоит посмотреть в первую очередь: те, что реверсят функции на **boot/стриминг/
  рендер-пути** (двигают standalone), а не изолированные игровые фичи.

## 3. 🧩 Task reverse-engineering backlog (из чек-листа пользователя)

### To-do (🔍 проверено 2026-10-11: файлов ещё НЕТ — задачи всё ещё актуальны)
- [ ] `CEntitySeekPosCalculatorDriving`
- [ ] `CEntitySeekPosCalculatorEntitySurface`
- [ ] `CEntitySeekPosCalculatorVehicleId`
- [ ] `CTaskInteriorSitInChair`
- [ ] `CTaskComplexFollowPatrolRoute`

### Finished (🔍 есть в дереве; большинство с 0 `plugin::Call` — т.е. реально реверснуты)
> ⚠️ Некоторые «finished» всё же содержат остаточный `plugin::Call` (напр. `CTaskComplexKillCriminal`,
> `CTaskComplexFallToDeath` — по 2) ⇒ их можно «дозачистить».

<details><summary>Список</summary>

- [x] `CTaskSimpleCarGoToPointNearDoorUntilDoorNotInUse`
- [x] `CTaskComplexOpenDriverDoor`
- [x] `CTaskComplexOpenPassengerDoor`
- [x] `CTaskComplexSeekEntityAiming`
- [x] `CTaskInteriorBeInHouse`
- [x] `CTaskInteriorBeInOffice`
- [x] `CTaskInteriorBeInShop`
- [x] `CTaskInteriorGoToInfo`
- [x] `CTaskInteriorLieInBed`
- [x] `CTaskInteriorShopKeeper`
- [x] `CTaskInteriorSitAtDesk`
- [x] `CTaskInteriorUseInfo`
- [x] `CTaskSimpleCarOpenLockedDoorFromOutside`
- [x] `CTaskSimpleCarSetPedSlowDraggedOut`
- [x] `CTaskSimpleCarShuffle`
- [x] `CTaskSimpleCarSlowBeDraggedOut`
- [x] `CTaskSimpleHurtPedWithCar`
- [x] `CTaskSimpleKillPedWithCar`
- [x] `CTaskComplexHitPedWithCar`
- [x] `CTaskSimpleDuckToggle` — Script
- [x] `CTaskSimpleFinishBrain` — Script
- [x] `CTaskSimpleOnEscalator`
- [x] `CTaskSimpleSetCharIgnoreWeaponRangeFlag`
- [x] `CTaskSimpleSetKindaStayInSamePlace`
- [x] `CTaskSimpleWaitUntilLeaderAreaCodesMatch`
- [x] `CTaskComplexKillCriminal`
- [x] `CTaskComplexFollowLeaderAnyMeans`
- [x] `CTaskComplexEnterCarAsDriverTimed`
- [x] `CTaskComplexEnterCarAsPassengerTimed`
- [x] `CTaskComplexFallToDeath`
- [x] `CTaskComplexWalkAlongsidePed`
- [x] `CTaskComplexEvasiveDiveAndGetUp`
- [x] `CTaskComplexUseClosestFreeScriptedAttractor`
- [x] `CTaskComplexUseClosestFreeScriptedAttractorRun`
- [x] `CTaskComplexUseClosestFreeScriptedAttractorSprint`
- [x] `CTaskComplexUseEffect`
- [x] `CTaskComplexUseEffectRunning`
- [x] `CTaskComplexUseEffectSprinting`
- [x] `CTaskComplexUsePairedAttractor`
- [x] `CTaskComplexUseAttractorPartner`
</details>

### Unused (не используются игрой — реверсить не обязательно)
<details><summary>Список</summary>

- `CTaskComplexFollowNodeRouteShooting`
- `CTaskComplexFallAndStayDown`
- `CTaskComplexExtinguishFireOnFoot`
- `CTaskSimpleRunAnimLoopedMiddle`
- `CTaskSimpleBeHitWhileMoving`
- `CTaskSimpleChat`
- `CTaskSimpleDuckWhileShotsWhizzing`
- `CTaskComplexGoToPointUntilCollisionAndStandStill`
- `CTaskComplexKillAllThreats`
- `CTaskComplexPresentIDToCop`
- `CTaskComplexReactToAttack`
- `CTaskSimpleHailTaxi`
- `CTaskSimpleHailTaxiAndPause`
- `CTaskComplexUseEntryExit`
- `CTaskSimpleWaitForPizza`
- `CTaskSimpleTriggerEvent`
</details>

## 4. 🗂️ Референсы для реверса (refs/)

> Всё в `refs/` **игнорируется гитом** (см. `.gitignore`, `refs/README.md`) — только для чтения, код оттуда не копируем.

### 5.1 Исходники других реверсов
- `refs/re3` (GTA III), `refs/reVC` (VC, ветка `miami`), `refs/librw` — см. `refs/README.md` и `MEMORY.md` §5b.
  Ближайшее существующее «как сделать standalone» на том же движке.
- **`refs/AdditionalSources/*`** (добавлены 2026-10-12, пользовательские — тоже только для чтения, не коммитим):
  * `SOME_GTA_SA_Sources` — **частичный реверс движка SA** (списки классов, битовые поля структур в `Docs/`,
    `Engine/C*.cpp`). Есть **`CSprite2d::AddToBuffer`** — то, чего нам не хватает для текста! `CFont` там
    частичный (`RenderFontBuffer` нет). Очень полезно для **layout'ов/структур/методов**.
  * `PartialSAMobileCode` — код **мобильной** SA: `pipes/` = **RxPipeline** (PC/MBR/Xbox + шейдеры `*.vsh`),
    ModelInfo, Camera, Ped, Automobile, script, MemoryMgr, TxdStore. Пригодится для наших заглушённых
    custom-пайплайнов и мобильной логики.
  * `SAMP` — **SA-MP 0.2.5** (клиент+сервер+RakNet). Для будущего **встроенного мультиплеера** (см. §6).
  * `VCSPC` — **Vice City Stories PC** (мод на движке SA; те же адреса `0x728200`/`0x7273D0`/`0x719840`, но
    реализации через `WRAPPER/EAXJMP` в оригинальный exe). Полезен как справочник по **логике** (`Frontend.cpp`,
    `MenuSystem.cpp`, `Hud.cpp`, `Renderer.cpp`, `Shaders/`), не по standalone-рендеру.

### 5.2 Оригинальный 1.0 US EXE + PDB + CrashList (`refs/gtasa10us`) 🆕
- **Что:** `gta_sa.exe` (14 383 616 байт = оригинальный **1.0 US**, НЕ Compact), фанатский `gta_sa.pdb`
  (небольшой — карта символов частичная), `CrashList.txt` (справочник «адрес → причина краха» из мод-мира).
- **Зачем (идеи):**
  * **Сверка адресов**: наш проект таргетит **Compact** exe (5 189 632 б), а это **1.0 US** — адреса могут отличаться.
    Полезно при разборе конкретных функций/адресов (сравнивать дизасм/смещения).
  * **Символы из PDB**: если он покрывает нужные функции — можно быстрее ориентироваться при реверсе.
  * **CrashList**: адрес падения → понятная причина (стриминг, лимиты, шейдеры и т.п.). Полезно при отладке
    падений standalone, когда стек ведёт в нереверснутую функцию.
- **Ограничения:** лицензионный бинарник (в репо/`git` не кладём — и так игнорируется); PDB неполный.
- **Статус:** 💡 идея использовать при разборе; пока не применялось.
- 💡 **Важно:** наш проект использует адреса **Compact** exe. У 1.0 US они могут быть смещены ⇒ при сверке
  сравниваем не «равен ли адрес», а **дизасм/смещение/сигнатуру функции**.

### 5.3 Портирование из апстрима
- См. §2 — портируем вручную (не через git), нужные PR — по запросу.

## 4. 🧱 Standalone-specific ideas

- 💡 **Заглушка-детектор**: чтобы оставшиеся `plugin::Call` в standalone не молча падали — логировать
  `"unimplemented GTA call at 0x..."` перед вызовом (сейчас это просто access violation).
- 💡 **Инвентарь `plugin::Call` по boot-пути** — построить карту вызовов от `WinMain` и приоритезировать
  реверс именно их (не по алфавиту).
- 💡 **RxPipeline-эффекты на librw.** SA использует классический RwPipeline API (`RxPipeline*`, `RxLockedPipeAddFragment`,
  `RxNodeDefinitionGetD3D9AtomicAllInOne`, …). В librw его нет (у librw своя, сильно упрощённая система пайплайнов:
  `rw::ObjPipeline`). Под `NOTSA_STANDALONE` поэтому **заглушены**: `CCarFXRenderer::Initialise` и
  `CCustomBuildingRenderer::Initialise` (косметика: env-map машин, кастомные здания). Позже — **реализовать эти
  эффекты поверх пайплайнов/плагинов librw** (отдельная крупная задача).
- 💡 **Аксессоры RW-полей** — свести все прямые обращения к полям RW-структур (см. `MEMORY.md` §10, ~115 мест)
  к одному слою, чтобы bridge был единственной точкой правды.
- ✅ **CFont / ТЕКСТ ВИДЕН — ЗАКРЫТО (2026-10-12).** Взят апстримный **PR #1414 «CFont done»** (ветка `fonts`,
  sha `82f34b0`): полный реверс `CFont` (`Font.cpp`/`Font.h`), `AddToBuffer` по спеке. Всё сверено с дизасмом.
  Меню рисуется настоящими шрифтами GTA (готика «Main Menu», «START GAME/OPTIONS/QUIT GAME», курсор, build-info).
  **Никакого TrueType-обхода не потребовалось** — либрв-путь рабочий, виноваты были **две наши** ошибки:
  * **Причина №1 — рассинхрон enum-значений (не только ID!).** `TranslateRenderState` переводил **ID** стейта
    (SA 29 → librw 21), но **не значение**. SA `RwAlphaTestFunction` = D3D-нумерация (`NEVER=1 … ALWAYS=8`),
    а librw `rw::AlphaTestFunc` = всего 3 значения (`ALWAYS=0, GREATEREQUAL=1, LESS=2`). `DefinedState2d`
    (дизасм 0x734750: `push 5; push 1Dh` ⇒ GREATER) индексировал `alphafuncMap[5]` **за границами массива** ⇒
    мусор ⇒ в логе `D3DCMP_LESS`, ref=2 ⇒ «проходить если alpha<2» ⇒ **непрозрачные пиксели глифов ОТБРАСЫВАЛИСЬ**,
    а прозрачные рисовались чёрным. Это объяснял ВСЕ симптомы: без текстуры — полосы видны; VINEWOOD (DXT1,
    `hasAlpha=0`) — виден; любой `hasAlpha=1` — нет (librw включает альфа-тест именно по `hasAlpha`).
    Фикс: `TranslateAlphaTestFunc`/`UntranslateAlphaTestFunc` в `RwBridge.cpp` + защитный clamp в
    `librw/src/d3d/d3ddevice.cpp` (`value>2 ⇒ 0`), чтобы такой рассинхрон больше не давал тихий мусор.
  * **Причина №2 — неинициализированные игровые глобалы в standalone.** `CHudColours` (0xBAB22C) — статик по
    фикс-адресу; в ASI-билде его конструирует CRT оригинального `gta_sa.exe`, в standalone область статиков
    просто обнулена. ⇒ таблица HUD-цветов нулевая ⇒ `SetDropColor(HudColour.GetRGB(HUD_COLOUR_BLACK))`
    ставил `(0,0,0,0)` ⇒ **`DropColor.a=0`**. А `CFont::PrintString` после отрисовки контура/тени восстанавливает
    **только `r,g,b`** (альфа остаётся от `DropColor`) ⇒ весь текст получал **alpha=0**. Доказано логом:
    `col=#ACCBF1**00**` при верных RGB. Фикс: `InitGameGlobalStatics()` в `WinMain.cpp` (standalone) вызывает
    `HudColour.Constructor()` (0x58FDA0) до `MainLoop`.
  **⚠️ Из этого вытекает отдельная задача (см. ниже про статические глобалы).**
  **Урок на будущее (в этот блок не возвращаться):** перепроверены и **НЕ** являются причиной — формат растра
  (`rwFormat=0x300`/DXT3, C4444), конвертация в C8888, бейк альфы в непрозрачный C888, свежая текстура/TrueType
  GDI-атлас, неиндексный флаш, `ZTEST`, `VERTEXALPHA`, тайминг внутри кадра, пустизна данных текстуры,
  `CSprite::FlushSpriteBuffer`. Всё это было исчерпывающе отсеяно; не тратить на это время снова.
- 💡 **`std::format` как источник падений (класс багов, искать дальше).** Апстрим активно заменяет оригинальные
  `sprintf`/`strcpy`/`AsciiToGxtChar(char*, …)` на `AsciiToGxtChar(std::format("{}", x).c_str(), …)`.
  **`std::format` бросает `std::format_error`, если аргумент — нулевой `char*`** (в отличие от printf/sprintf,
  которые просто печатают `(null)`). Значит любое такое место с потенциально нулевым указателем = падение
  всего процесса (0xe06d7363, необработанное исключение). Уже найдено и исправлено:
  `CMenuManager::DrawStandardMenus` / `MENU_ACTION_RESOLUTION` (MenuManager_Draw.cpp:652, см. `VERIFY.md` §C).
  **Стоит пройтись по всем `std::format("{}", <указатель/элемент массива/Get…>)`** в меню/HUD/страницах
  (`MenuManager_Pages.cpp`, `Hud.cpp`, `Frontend/*`) — особенно там, где данные приходят из таблиц/списков,
  которые могут содержать пропуски (видеорежимы, текстурные имена, `TheText.Get` несуществующего ключа).
- 💡 **Статические глобалы с C++-конструкторами (новое, важно).** Причина №2 — **системная**: любой глобал по
  фикс-адресу, у которого есть конструктор, в standalone обнулён. Кандидаты (у них есть `Constructor()`):
  `CText`, `CPlayerInfo`, `CPlayerPedData`, `CPedClothesDesc`, `CMouseControllerState`, `CSprite2d[]`,
  `CCamera`, `CDirectory`, `CFire(Manager)`, `CEvent`, `CInterestingEvents`, `Fx_c`/`FxManager_c`/`FxSystem(_BP)_c`,
  `CMenuManager`, `CScriptedBrainTaskEntry`, `CPedScriptedTaskRecordData`, задачи `CTask*`.
  **Сделать:** единый реестр «статик → конструктор» (например макрос/список в `WinMain.cpp`) + вызов до `MainLoop`,
  и сверить с дизасмом, какие из них реально инициализируются статическими CRT-конструкторами оригинала.
  Пока этого нет — ожидаем «нулевые» поля в нераскрученных подсистемах (цвета, размеры, счётчики).
  **Прогресс (2026-10-12):** просканировано дерево — 2352 сайта `StaticRef`, 641 уникальный тип, из них
  **39 полиморфных/с конструктором** (список в выводе скана: `CCamera`, `CPlayerInfo`, `CFireManager`,
  `CMenuManager`, `CText`, `cHandlingDataMgr`, `CAudioEngine`/`CAE*`, `FxManager_c`, `CColModel`×18,
  `CVector`×56/`CRGBA`×9/`CMatrix`×3 — последние POD-подобные, нули им не вредят). Уже сконструированы
  в `InitGameGlobalStatics()`: `CHudColours` + 9 сторов моделей (`CModelInfo::ms_*ModelInfoStore`,
  через новый `notsa::ConstructStatic` в `Base.h`). `ms_2dFXInfoStore` намеренно пропущен (`C2dEffect`
  не полиморфен — тег `e2dEffectType`, и он к тому же не default-конструируем: анонимное объединение).
- 💡 **Альфа-тест: потеря точности перевода (мелочь, но записать).** SA/RW-нумерация `RwAlphaTestFunction`
  **совпадает с `D3DCMPFUNC`**, поэтому вендорный D3D9-драйвер RW делал идентичный проброс всех 8 значений,
  а librw имеет только 3 (`ALWAYS/GREATEREQUAL/LESS`) — мой перевод теряет точность (`GREATER→GREATEREQUAL`,
  `EQUAL/NOTEQUAL→ALWAYS`, `NEVER→LESS`). Для шрифта (`GREATER`, ref=2) поведение совпадает с оригиналом
  (255>2 ⇔ 255≥2), но для строгости стоит добавить в librw D3D-путую «обходную» установку `D3DRS_ALPHAFUNC`
  сымым значением RW-enum (либо расширить map до 9, не трогая `AlphaTestFunc`, используемый GS-эмуляцией).
  **Полная цепочка текста (нужно реализовать всё):**
  1. `CFont::PrintChar` (0x718A10) — есть тело под заглушкой (не хватает лишь `else if (RenderState.m_bContainImages)`).
  2. `CSprite2d::AddToBuffer` (0x728200) — **не реверснут**; кодит `CFontChar` (12 × 0x30) в `FontRenderStateBuf`
     и глифы в буфер `aRadiosityVertexBuffer`. Нужен реверс (посмотреть как `RenderVertexBuffer` рисует).
  3. `CFont::ProcessCurrentString` (0x71A220) — разбор строки/токенов → `PrintChar` (сейчас no-op).
  4. `CFont::RenderFontBuffer` (0x719840) — финальный проход по `FontRenderStateBuf` → `CSprite2d` draws + сброс `pEmptyChar`.
  Референс: `refs/reVC/src/renderer/Font.cpp` (там `PrintChar`/`RenderFontBuffer` есть, но VC-структуры отличаются).
  **+ `refs/AdditionalSources/SOME_GTA_SA_Sources/Engine/CSprite2d.cpp` — там `AddToBuffer` УЖЕ реализован** (см. §5.1).
  Проверять `FontRenderStateBuf` (`CFontChar[9]`, адрес 0xC716B0) и `pEmptyChar` (0xC716A8).
- 💡 **`RwIm3D*` на librw.** `RwIm3DTransform/RenderPrimitive/RenderIndexedPrimitive/RenderLine/End`
  (`rwcore.cpp`, адреса 0x7EF450…) пока вызываются в оригинальный exe. Их много (эффекты, следы, стёкла,
  линии отладки). Нужен маппинг на librw `im3d::*` — но у librw другой layout `Im3DVertex` (position/normal/color/uv),
  проверить совместимость. Сейчас — **не сделано** (следующая крупная RW-задача после текста).
- 💡 **Аудио (sound playback) не работает в standalone.** Сейчас заглушены: `CAESoundManager::Service`
  (иначе крэш на `m_AudioEntity->UpdateParameters`), `CAERadioTrackManager::Service`. Причина — где-то
  `CAESound::m_AudioEntity` оказывается мусорным (возможно несовместимость layout/reinterpret_cast в audio-сущностях,
  либо неинициализированный `AudioEntity`). Нужно разобрать весь audio-путь (и `CAEAudioEntity` vtable/размеры —
  `VALIDATE_SIZE` только для размера, не для полей). Как всегда — сверяться с `refs/reVC` (`src/audio/`).
- 💡 **Проверить семантику decision-maker'ов.** Под standalone реализованы (best-effort) `CDecision::SetDefault/Set`,
  `CDecisionMakerTypes::{GetInstance,LoadEventIndices,AddDecisionMaker,RemoveDecisionMaker}` (в оригинале — `plugin::Call`).
  Значения (особенно `Set` float→uint8, `AddDecisionMaker` возвращаемый script-handle, `SetDefault` дефолтные задачи)
  нужно **сверить с оригиналом** (сейчас задача = `TASK_NONE`, вероятности 0..100, маппинг событий — по именам).
  На boot не влияет (затрагивает только ИИ педов), но перед реальной игрой — перепроверить.

## 6. 🌐 Встроенный мультиплеер (future)

> **Идея пользователя:** сделать **встроенный мультиплеер** прямо в standalone-игре (не внешний SA-MP).

- 💡 **Референс протокола/клиента:** `refs/AdditionalSources/SAMP` (SA-MP 0.2.5 client/server + RakNet). Смотреть
  `client/net/` (raknet-обёртка), `client/game/` (хуки на игровые функции), `client/gui` (chat/labels/tags),
  `server/` (gamemode/скрипты). Это «как SA-MP внедряется в игру» — но у нас игра уже **наша собственная**, поэтому
  можно делать чище (свои менеджеры игроков/снапшотов, без memory-хуков).
- 💡 **re3/GameSpy:** в `refs/re3`/`reVC` от оригинального сетевого кода остался только рудимент — действие
  `NETWORK_TALK` в `ControllerConfig` (+ пара упоминаний). Полноценной реализации GameSpy там **нет**, «левтоферсов»
  почти не осталось — рассчитывать на них не стоит, писать своё.
- 💡 **Транспорт:** RakNet (как в SAMP) или современнее — `ENet`/`GameNetworkingSockets`. SA-MP-протокол можно взять
  за образец сообщений (RPC/sync), но лучше опереться на уже готовую сетевую библиотеку.
- 💡 **Архитектура (черновик):** отдельный модуль `net/` в standalone: клиент (подключение, приём снапшотов,
  интерполяция сущностей) + сервер (авторитетная симуляция игроков/машин, стриминг мира). Синхронизация через
  уже существующие `CPlayerPed`/`CVehicle` (по аналогии с тем, как это делается в SA-MP, но без чужих хук-хаков).
- 💡 **UI:** чат/ник-теги/список игроков — можно подсмотреть в `refs/AdditionalSources/SAMP/client` (`chatwindow`,
  `playertags`, `label`) и использовать наш `CFont`/`CSprite2d` (когда текст заведём — см. §4).
- 🚫 **Не сейчас.** Сначала — одиночный boot/геймплей. Мультиплеер — после того как мир грузится и играется.
