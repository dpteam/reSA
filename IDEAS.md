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
- 💡 **Проверить семантику decision-maker'ов.** Под standalone реализованы (best-effort) `CDecision::SetDefault/Set`,
  `CDecisionMakerTypes::{GetInstance,LoadEventIndices,AddDecisionMaker,RemoveDecisionMaker}` (в оригинале — `plugin::Call`).
  Значения (особенно `Set` float→uint8, `AddDecisionMaker` возвращаемый script-handle, `SetDefault` дефолтные задачи)
  нужно **сверить с оригиналом** (сейчас задача = `TASK_NONE`, вероятности 0..100, маппинг событий — по именам).
  На boot не влияет (затрагивает только ИИ педов), но перед реальной игрой — перепроверить.
