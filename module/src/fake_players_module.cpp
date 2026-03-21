#include "fake_players_module.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace fmp {

struct TimeRule {
    int from_minutes{};
    int to_minutes{};
    int fake_min{};
    int fake_max{};
};

struct OnlineRule {
    int real_min{};
    int real_max{};
    int fake_min{};
    int fake_max{};
};

struct Config {
    bool dynamic_mode{false};
    int static_count{12};
    int dynamic_base{10};
    int dynamic_add{8};
    int dynamic_divisor{2};
    std::vector<TimeRule> time_rules;
    std::vector<OnlineRule> online_rules;
    std::vector<std::string> nicknames;
};

struct Snapshot {
    int announced_players{};
    int announced_bots{};
    int fake_players{};
    std::vector<std::string> announced_nicknames;
};

class Engine {
public:
    bool reload(const std::string& schedule_path, const std::string& nicks_path) {
        return load_schedule(schedule_path) && load_nicks(nicks_path);
    }

    Snapshot build_snapshot(int real_players) {
        Snapshot out{};
        const int fake_count = compute_target(real_players);
        out.fake_players = std::max(0, fake_count);
        out.announced_players = std::max(0, real_players) + out.fake_players;
        out.announced_bots = 0;
        out.announced_nicknames.reserve(static_cast<std::size_t>(out.fake_players));

        for (int i = 0; i < out.fake_players; ++i) {
            out.announced_nicknames.push_back(next_nick());
        }

        last_snapshot_ = out;
        return out;
    }

    size_t nick_count() const {
        return last_snapshot_.announced_nicknames.size();
    }

    const char* nick_at(size_t index) const {
        if (index >= last_snapshot_.announced_nicknames.size()) {
            return nullptr;
        }
        return last_snapshot_.announced_nicknames[index].c_str();
    }

private:
    Config cfg_{};
    Snapshot last_snapshot_{};
    std::size_t nick_cursor_{0};
    std::mt19937 rng_{std::random_device{}()};

    static std::string trim(std::string value) {
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
            value.pop_back();
        }

        std::size_t offset = 0;
        while (offset < value.size() && std::isspace(static_cast<unsigned char>(value[offset])) != 0) {
            ++offset;
        }

        return value.substr(offset);
    }

    static bool in_range_wrap(int now, int from, int to) {
        if (from <= to) {
            return now >= from && now <= to;
        }
        return now >= from || now <= to;
    }

    int random_range(int a, int b) {
        if (b < a) {
            std::swap(a, b);
        }
        std::uniform_int_distribution<int> dist(a, b);
        return dist(rng_);
    }

    static int parse_clock_to_minutes(const std::string& hhmm) {
        const auto pos = hhmm.find(':');
        if (pos == std::string::npos) {
            return -1;
        }

        const int h = std::stoi(hhmm.substr(0, pos));
        const int m = std::stoi(hhmm.substr(pos + 1));
        if (h < 0 || h > 23 || m < 0 || m > 59) {
            return -1;
        }
        return h * 60 + m;
    }

    static int now_minutes() {
        const auto current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm local_time{};
    #if defined(_WIN32)
        localtime_s(&local_time, &current_time);
    #else
        localtime_r(&current_time, &local_time);
    #endif
        return local_time.tm_hour * 60 + local_time.tm_min;
    }

    int compute_target(int real_players) {
        int target = cfg_.dynamic_mode
            ? cfg_.dynamic_base + std::max(0, cfg_.dynamic_add - std::max(0, real_players) / std::max(1, cfg_.dynamic_divisor))
            : cfg_.static_count;

        int selected = -1;
        const int now = now_minutes();

        for (const auto& rule : cfg_.time_rules) {
            if (in_range_wrap(now, rule.from_minutes, rule.to_minutes)) {
                selected = random_range(rule.fake_min, rule.fake_max);
                break;
            }
        }

        for (const auto& rule : cfg_.online_rules) {
            if (real_players >= rule.real_min && real_players <= rule.real_max) {
                selected = random_range(rule.fake_min, rule.fake_max);
                break;
            }
        }

        if (selected >= 0) {
            target = selected;
        }

        return std::max(0, target);
    }

    std::string next_nick() {
        if (cfg_.nicknames.empty()) {
            return "Player_" + std::to_string(random_range(1000, 9999));
        }

        if (nick_cursor_ >= cfg_.nicknames.size()) {
            nick_cursor_ = 0;
        }

        return cfg_.nicknames[nick_cursor_++];
    }

    bool load_nicks(const std::string& path) {
        cfg_.nicknames.clear();

        std::ifstream input(path);
        if (!input.is_open()) {
            return false;
        }

        std::string line;
        while (std::getline(input, line)) {
            line = trim(line);
            if (line.empty() || line[0] == ';' || line[0] == '#') {
                continue;
            }
            cfg_.nicknames.push_back(line);
        }

        nick_cursor_ = 0;
        return true;
    }

    bool load_schedule(const std::string& path) {
        cfg_.time_rules.clear();
        cfg_.online_rules.clear();

        std::ifstream input(path);
        if (!input.is_open()) {
            return false;
        }

        std::string line;
        while (std::getline(input, line)) {
            line = trim(line);
            if (line.empty() || line[0] == ';' || line[0] == '#') {
                continue;
            }

            std::istringstream stream(line);
            std::string type;
            stream >> type;

            if (type == "TIME") {
                std::string range;
                TimeRule rule{};
                stream >> range >> rule.fake_min >> rule.fake_max;
                const auto dash = range.find('-');
                if (dash == std::string::npos) {
                    continue;
                }
                rule.from_minutes = parse_clock_to_minutes(range.substr(0, dash));
                rule.to_minutes = parse_clock_to_minutes(range.substr(dash + 1));
                if (rule.from_minutes < 0 || rule.to_minutes < 0) {
                    continue;
                }
                cfg_.time_rules.push_back(rule);
                continue;
            }

            if (type == "ONLINE") {
                OnlineRule rule{};
                stream >> rule.real_min >> rule.real_max >> rule.fake_min >> rule.fake_max;
                cfg_.online_rules.push_back(rule);
            }
        }

        return true;
    }
};

} // namespace fmp

struct fmp_handle_s {
    fmp::Engine engine;
};

extern "C" {

fmp_handle_t* fmp_create(void) {
    return new fmp_handle_s();
}

void fmp_destroy(fmp_handle_t* handle) {
    delete handle;
}

int fmp_reload(fmp_handle_t* handle, const char* schedule_path, const char* nicks_path) {
    if (handle == nullptr || schedule_path == nullptr || nicks_path == nullptr) {
        return 0;
    }
    return handle->engine.reload(schedule_path, nicks_path) ? 1 : 0;
}

int fmp_make_snapshot(fmp_handle_t* handle, int real_players, fmp_snapshot_t* snapshot) {
    if (handle == nullptr || snapshot == nullptr) {
        return 0;
    }

    const auto value = handle->engine.build_snapshot(real_players);
    snapshot->announced_players = value.announced_players;
    snapshot->announced_bots = value.announced_bots;
    snapshot->fake_players = value.fake_players;
    return 1;
}

const char* fmp_snapshot_nick_at(fmp_handle_t* handle, size_t index) {
    if (handle == nullptr) {
        return nullptr;
    }
    return handle->engine.nick_at(index);
}

size_t fmp_snapshot_nick_count(fmp_handle_t* handle) {
    if (handle == nullptr) {
        return 0;
    }
    return handle->engine.nick_count();
}

} // extern "C"
