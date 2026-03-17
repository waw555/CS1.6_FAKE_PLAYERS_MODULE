# CS 1.6 Fake Players **Module** (Metamod-R / ReHLDS)

Этот репозиторий теперь ориентирован на **серверный модуль**, а не на AMX Mod X плагин.

Цель: формировать данные для `A2S_INFO` / `A2S_PLAYER`, чтобы мониторинги Steam видели настраиваемый онлайн и список ников без необходимости держать fake clients в слотах сервера.

## Что реализовано

- Ядро модуля на C++ (`module/src/fake_players_module.cpp`):
  - статический/динамический расчет фейкового онлайна;
  - правила по времени (`TIME`) и реальному онлайну (`ONLINE`);
  - загрузка ников из отдельного файла;
  - подготовка «снимка» (snapshot) с целевыми значениями: players/bots/nicks.
- C ABI-функции для будущей интеграции в Metamod-R/ReHLDS glue:
  - `fmp_create`, `fmp_destroy`, `fmp_reload`, `fmp_make_snapshot`.

## Важно

Текущий код — это **core для модуля + экспорт API**, который нужно подключить к вашему обработчику A2S на уровне Metamod-R/ReHLDS.

То есть:
- логика расчета и конфиги готовы;
- сериализация UDP-ответов A2S и engine hooks должны быть добавлены в вашем слое интеграции (или следующим шагом в этом репозитории).

## Конфиги

### `configs/fake_monitor_players.ini`

```ini
; TIME HH:MM-HH:MM fake_min fake_max
TIME 00:00-08:59 14 18
TIME 09:00-17:59 8 12
TIME 18:00-23:59 10 16

; ONLINE real_min real_max fake_min fake_max
ONLINE 0 5 12 18
ONLINE 6 12 8 12
ONLINE 13 20 4 8
ONLINE 21 32 0 2
```

- `ONLINE` имеет приоритет над `TIME`.

### `configs/fake_monitor_nicks.ini`

Один ник на строку.

## Сборка (Debian Linux)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Результат: `build/libfake_players_module.so`

## Следующий шаг для полного A2S-модуля

1. Подключить `.so` в ваш Metamod-R/ReHLDS проект.
2. В query hooks:
   - в `A2S_INFO` подставлять `announcedPlayers`/`announcedBots`;
   - в `A2S_PLAYER` формировать список игроков из snapshot-ников.
3. На reload карты/конфига вызывать `fmp_reload(...)`.

---

Если хотите, следующим коммитом я добавлю именно glue-слой под конкретный API ReHLDS/Metamod-R (под ваши заголовки и версию SDK), чтобы это был полностью рабочий бинарный модуль под ваш сервер.
