# IDEAS.md — backlog & raw ideas

> **Что это:** сырой список идей/задач на будущее. Здесь **ничего не обязано быть реализовано** и ничего
> отсюда не является «планом». Когда что-то отсюда берётся в работу — оно переезжает в `PROGRESS.md`
> (или в трекер задачи), а сюда добавляется пометка «→ взято в работу».
>
> Статусы: 💡 идея · 🔍 проверено · ✅ сделано · 🚫 отклонено

---

## 1. 🎮 Фичи, которые хочется реализовать (general)

- 💡 **Оконный режим по умолчанию + корректная работа по RDP** — сейчас в standalone оконный режим не
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

## 4. 🧱 Standalone-specific ideas

- 💡 **Заглушка-детектор**: чтобы оставшиеся `plugin::Call` в standalone не молча падали — логировать
  `"unimplemented GTA call at 0x..."` перед вызовом (сейчас это просто access violation).
- 💡 **Инвентарь `plugin::Call` по boot-пути** — построить карту вызовов от `WinMain` и приоритезировать
  реверс именно их (не по алфавиту).
- 💡 **Аксессоры RW-полей** — свести все прямые обращения к полям RW-структур (см. `MEMORY.md` §10, ~115 мест)
  к одному слою, чтобы bridge был единственной точкой правды.
