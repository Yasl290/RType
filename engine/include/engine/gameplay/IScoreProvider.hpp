#pragma once

#include <unordered_map>
#include <cstdint>

struct PlayerScore {
    uint32_t client_id;
    uint32_t score;
    uint32_t enemies_killed;
};

class IScoreProvider {
public:
    virtual ~IScoreProvider() = default;
    virtual std::unordered_map<uint32_t, PlayerScore> getPlayerScores() const = 0;
};