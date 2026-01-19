#include "LocalScoreProvider.hpp"
#include <engine/gameplay/Controllable.hpp>
#include <engine/gameplay/Score.hpp>

LocalScoreProvider::LocalScoreProvider(Registry& registry)
    : _registry(registry)
{
}

std::unordered_map<uint32_t, PlayerScore> LocalScoreProvider::getPlayerScores() const {
    std::unordered_map<uint32_t, PlayerScore> scores;
    
    _registry.each<Controllable, Score>([&](EntityID, Controllable&, Score& score) {
        PlayerScore ps;
        ps.client_id = 0;
        ps.score = score.getPoints();
        ps.enemies_killed = score.getEnemiesKilled();
        scores[0] = ps;
    });
    
    return scores;
}