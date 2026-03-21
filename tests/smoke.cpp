#include "fake_players_module.h"

#include <iostream>

int main() {
    fmp_handle_t* handle = fmp_create();
    if (handle == nullptr) {
        std::cerr << "failed to create handle\n";
        return 1;
    }

    const char* schedule = FMP_TEST_SOURCE_DIR "/configs/fake_monitor_players.ini";
    const char* nicks = FMP_TEST_SOURCE_DIR "/configs/fake_monitor_nicks.ini";
    const int reloaded = fmp_reload(handle, schedule, nicks);
    if (reloaded != 1) {
        std::cerr << "failed to load config files\n";
        fmp_destroy(handle);
        return 2;
    }

    fmp_snapshot_t snapshot{};
    if (fmp_make_snapshot(handle, 4, &snapshot) != 1) {
        std::cerr << "failed to build snapshot\n";
        fmp_destroy(handle);
        return 3;
    }

    std::cout << "announced_players=" << snapshot.announced_players << " fake_players=" << snapshot.fake_players << '\n';
    for (size_t i = 0; i < fmp_snapshot_nick_count(handle); ++i) {
        const char* nick = fmp_snapshot_nick_at(handle, i);
        if (nick != nullptr) {
            std::cout << nick << '\n';
        }
    }

    fmp_destroy(handle);
    return 0;
}
