#pragma once

#include <engine/gameplay/IScoreProvider.hpp>
#include <engine/core/Registry.hpp>

class LocalScoreProvider : public IScoreProvider {
public:
    LocalScoreProvider(Registry& registry);
    
    std::unordered_map<uint32_t, PlayerScore> getPlayerScores() const override;

private:
    Registry& _registry;
};