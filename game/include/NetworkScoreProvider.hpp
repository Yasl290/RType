#pragma once

#include <engine/gameplay/IScoreProvider.hpp>
#include <unordered_map>
#include <cstdint>

class NetworkClient;

class NetworkScoreProvider : public IScoreProvider {
public:
    explicit NetworkScoreProvider(NetworkClient& client);
    
    std::unordered_map<uint32_t, PlayerScore> getPlayerScores() const override;

private:
    NetworkClient& _client;
};