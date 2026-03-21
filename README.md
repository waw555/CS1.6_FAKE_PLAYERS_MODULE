# CS 1.6 Fake Players Module

Модуль собирается в `libfake_players_module.so` и предоставляет C ABI для слоя интеграции с ReHLDS / Metamod-R / ReGameDLL.

## Что делает модуль

- Загружает правила из `configs/fake_monitor_players.ini`.
- Загружает список ников из `configs/fake_monitor_nicks.ini`.
- По реальному онлайну строит snapshot с:
  - итоговым количеством игроков для анонса в `A2S_INFO`;
  - количеством фейковых игроков;
  - списком фейковых ников для `A2S_PLAYER`.
- Экспортирует стабильный C API, который можно дергать из glue-слоя Metamod-R.

## Ограничение

Этот репозиторий собирает именно **ядро модуля**. Для подмены ответов в Steam query и отправки измененных данных наружу нужен слой интеграции, который будет вызывать API модуля внутри query hooks вашего ReHLDS/Metamod-R проекта.

То есть реализация логики готова, но точка встраивания зависит от конкретного SDK/заголовков вашего сервера.

## C API

```c
fmp_handle_t* fmp_create(void);
void fmp_destroy(fmp_handle_t* handle);
int fmp_reload(fmp_handle_t* handle, const char* schedule_path, const char* nicks_path);
int fmp_make_snapshot(fmp_handle_t* handle, int real_players, fmp_snapshot_t* snapshot);
const char* fmp_snapshot_nick_at(fmp_handle_t* handle, size_t index);
size_t fmp_snapshot_nick_count(fmp_handle_t* handle);
```

## Сборка в Debian 13

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Интеграция с Metamod-R / ReHLDS

Типовой сценарий:

1. Создать `fmp_handle_t*` при старте модуля.
2. Вызвать `fmp_reload(...)` после загрузки конфигов или по server command reload.
3. Перед формированием `A2S_INFO` вызвать `fmp_make_snapshot(...)`, передав реальное число игроков.
4. Использовать `snapshot.announced_players` как количество игроков в ответе.
5. Использовать `fmp_snapshot_nick_count()` и `fmp_snapshot_nick_at()` для заполнения списка игроков в `A2S_PLAYER`.

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

- `ONLINE` rules имеют приоритет над `TIME`.
- Если правила не совпали, используется базовое статическое значение.
