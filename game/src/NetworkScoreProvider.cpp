#include "NetworkScoreProvider.hpp"
#include "NetworkClient.hpp"

NetworkScoreProvider::NetworkScoreProvider(NetworkClient& client)
    : _client(client)
{
}

std::unordered_map<uint32_t, PlayerScore> NetworkScoreProvider::getPlayerScores() const
{
    return _client.getPlayerScores();
}