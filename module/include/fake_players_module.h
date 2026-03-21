#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fmp_handle_s fmp_handle_t;

typedef struct fmp_snapshot_s {
    int announced_players;
    int announced_bots;
    int fake_players;
} fmp_snapshot_t;

fmp_handle_t* fmp_create(void);
void fmp_destroy(fmp_handle_t* handle);

int fmp_reload(fmp_handle_t* handle, const char* schedule_path, const char* nicks_path);
int fmp_make_snapshot(fmp_handle_t* handle, int real_players, fmp_snapshot_t* snapshot);
const char* fmp_snapshot_nick_at(fmp_handle_t* handle, size_t index);
size_t fmp_snapshot_nick_count(fmp_handle_t* handle);

#ifdef __cplusplus
}
#endif
