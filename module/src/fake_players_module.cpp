#include <algorithm>
#include <chrono>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace fmp {

struct TimeRule {
    int fromMinutes{};
    int toMinutes{};
    int fakeMin{};
    int fakeMax{};
};

struct OnlineRule {
    int realMin{};
    int realMax{};
    int fakeMin{};
    int fakeMax{};
};

struct Config {
    bool dynamicMode{false};
    int staticCount{12};
    int dynamicBase{10};
    int dynamicAdd{8};
    int dynamicDivisor{2};
    std::vector<TimeRule> timeRules;
    std::vector<OnlineRule> onlineRules;
    std::vector<std::string> nicknames;
};

struct Snapshot {
    int announcedPlayers{};
    int announcedBots{};
    std::vector<std::string> announcedNicknames;
};

class Engine {
public:
    void reload(const std::string& schedulePath, const std::string& nicksPath) {
        loadSchedule(schedulePath);
        loadNicks(nicksPath);
    }

    Snapshot buildSnapshot(int realPlayers) {
        Snapshot out{};
        int fakeCount = computeTarget(realPlayers);
        out.announcedPlayers = std::max(0, realPlayers + fakeCount);
        out.announcedBots = 0; // for A2S_INFO/A2S_PLAYER response shaping

        for (int i = 0; i < fakeCount; ++i) {
            out.announcedNicknames.push_back(nextNick());
        }
        return out;
    }

private:
    Config cfg_{};
    std::size_t nickCursor_{0};
    std::mt19937 rng_{std::random_device{}()};

    static std::string trim(std::string s) {
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
        std::size_t i = 0;
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
        return s.substr(i);
    }

    static bool inRangeWrap(int now, int from, int to) {
        if (from <= to) return now >= from && now <= to;
        return now >= from || now <= to;
    }

    int randomRange(int a, int b) {
        if (b < a) std::swap(a, b);
        std::uniform_int_distribution<int> dist(a, b);
        return dist(rng_);
    }

    int parseClockToMinutes(const std::string& hhmm) {
        auto pos = hhmm.find(':');
        if (pos == std::string::npos) return -1;
        int h = std::stoi(hhmm.substr(0, pos));
        int m = std::stoi(hhmm.substr(pos + 1));
        if (h < 0 || h > 23 || m < 0 || m > 59) return -1;
        return h * 60 + m;
    }

    int nowMinutes() const {
        auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm local{};
#if defined(_WIN32)
        localtime_s(&local, &t);
#else
        localtime_r(&t, &local);
#endif
        return local.tm_hour * 60 + local.tm_min;
    }

    int computeTarget(int realPlayers) {
        int target = cfg_.dynamicMode
            ? cfg_.dynamicBase + std::max(0, cfg_.dynamicAdd - realPlayers / std::max(1, cfg_.dynamicDivisor))
            : cfg_.staticCount;

        int selected = -1;
        int now = nowMinutes();
        for (const auto& rule : cfg_.timeRules) {
            if (inRangeWrap(now, rule.fromMinutes, rule.toMinutes)) {
                selected = randomRange(rule.fakeMin, rule.fakeMax);
                break;
            }
        }
        for (const auto& rule : cfg_.onlineRules) {
            if (realPlayers >= rule.realMin && realPlayers <= rule.realMax) {
                selected = randomRange(rule.fakeMin, rule.fakeMax);
                break;
            }
        }
        if (selected >= 0) target = selected;
        return std::max(0, target);
    }

    std::string nextNick() {
        if (cfg_.nicknames.empty()) {
            return "Player_" + std::to_string(randomRange(1000, 9999));
        }
        if (nickCursor_ >= cfg_.nicknames.size()) nickCursor_ = 0;
        return cfg_.nicknames[nickCursor_++];
    }

    void loadNicks(const std::string& path) {
        cfg_.nicknames.clear();
        std::ifstream in(path);
        std::string line;
        while (std::getline(in, line)) {
            line = trim(line);
            if (line.empty() || line[0] == ';' || line[0] == '#') continue;
            cfg_.nicknames.push_back(line);
        }
        nickCursor_ = 0;
    }

    void loadSchedule(const std::string& path) {
        cfg_.timeRules.clear();
        cfg_.onlineRules.clear();

        std::ifstream in(path);
        std::string line;
        while (std::getline(in, line)) {
            line = trim(line);
            if (line.empty() || line[0] == ';' || line[0] == '#') continue;

            std::istringstream ss(line);
            std::string type;
            ss >> type;

            if (type == "TIME") {
                std::string range;
                TimeRule r{};
                ss >> range >> r.fakeMin >> r.fakeMax;
                auto dash = range.find('-');
                if (dash == std::string::npos) continue;
                r.fromMinutes = parseClockToMinutes(range.substr(0, dash));
                r.toMinutes = parseClockToMinutes(range.substr(dash + 1));
                if (r.fromMinutes < 0 || r.toMinutes < 0) continue;
                cfg_.timeRules.push_back(r);
            } else if (type == "ONLINE") {
                OnlineRule r{};
                ss >> r.realMin >> r.realMax >> r.fakeMin >> r.fakeMax;
                cfg_.onlineRules.push_back(r);
            }
        }
    }
};

} // namespace fmp

// --- exported C ABI for Metamod/ReHLDS glue layer ---
extern "C" {

void* fmp_create() {
    return new fmp::Engine();
}

void fmp_destroy(void* handle) {
    delete static_cast<fmp::Engine*>(handle);
}

void fmp_reload(void* handle, const char* schedulePath, const char* nicksPath) {
    if (!handle || !schedulePath || !nicksPath) return;
    static_cast<fmp::Engine*>(handle)->reload(schedulePath, nicksPath);
}

int fmp_make_snapshot(void* handle, int realPlayers, int* announcedPlayers, int* announcedBots) {
    if (!handle || !announcedPlayers || !announcedBots) return 0;
    auto snapshot = static_cast<fmp::Engine*>(handle)->buildSnapshot(realPlayers);
    *announcedPlayers = snapshot.announcedPlayers;
    *announcedBots = snapshot.announcedBots;
    return static_cast<int>(snapshot.announcedNicknames.size());
}

// NOTE: Integration point.
// A real Metamod-R/ReHLDS module should call fmp_make_snapshot()
// inside A2S_INFO/A2S_PLAYER query hooks and serialize snapshot data to UDP response.

}
