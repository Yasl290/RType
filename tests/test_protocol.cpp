#include <gtest/gtest.h>
#include "protocol/Protocol.hpp"
#include <cstring>

using namespace RType::Protocol;

TEST(ProtocolTest, PacketHeaderMagic) {
    PacketHeader header;
    EXPECT_EQ(header.magic, 0x52545950);
}

TEST(ProtocolTest, IsValidPacket) {
    PacketHeader header;
    EXPECT_TRUE(IsValidPacket(header));
    
    header.magic = 0x12345678;
    EXPECT_FALSE(IsValidPacket(header));
}

TEST(ProtocolTest, ConnectRequestSize) {
    ConnectRequest request;
    EXPECT_EQ(request.header.type, PacketType::CONNECT_REQUEST);
    EXPECT_GT(sizeof(ConnectRequest), sizeof(PacketHeader));
}

TEST(ProtocolTest, ConnectRequestInitialization) {
    ConnectRequest request;
    std::strncpy(request.client_version, "1.0.0", sizeof(request.client_version) - 1);
    std::strncpy(request.player_name, "TestPlayer", sizeof(request.player_name) - 1);
    
    EXPECT_STREQ(request.client_version, "1.0.0");
    EXPECT_STREQ(request.player_name, "TestPlayer");
}

TEST(ProtocolTest, ConnectResponseStatus) {
    ConnectResponse response;
    EXPECT_EQ(response.status, ConnectionStatus::ACCEPTED);
    EXPECT_EQ(response.header.type, PacketType::CONNECT_RESPONSE);
}

TEST(ProtocolTest, PlayerInputFlags) {
    PlayerInput input;
    input.input_flags = 0x01 | 0x02 | 0x10;
    
    EXPECT_TRUE(input.input_flags & 0x01);
    EXPECT_TRUE(input.input_flags & 0x02);
    EXPECT_FALSE(input.input_flags & 0x04);
    EXPECT_FALSE(input.input_flags & 0x08);
    EXPECT_TRUE(input.input_flags & 0x10);
}

TEST(ProtocolTest, EntityUpdateInitialization) {
    EntityUpdate update;
    update.entity_id = 42;
    update.pos_x = 100.f;
    update.pos_y = 200.f;
    update.vel_x = 10.f;
    update.vel_y = 5.f;
    update.entity_type = 1;
    update.hp_current = 50.f;
    update.hp_max = 100.f;
    update.player_slot = 2;
    
    EXPECT_EQ(update.entity_id, 42);
    EXPECT_FLOAT_EQ(update.pos_x, 100.f);
    EXPECT_FLOAT_EQ(update.pos_y, 200.f);
    EXPECT_FLOAT_EQ(update.vel_x, 10.f);
    EXPECT_FLOAT_EQ(update.vel_y, 5.f);
    EXPECT_EQ(update.entity_type, 1);
    EXPECT_FLOAT_EQ(update.hp_current, 50.f);
    EXPECT_FLOAT_EQ(update.hp_max, 100.f);
    EXPECT_EQ(update.player_slot, 2);
}

TEST(ProtocolTest, LobbyStatusInitialization) {
    LobbyStatus status;
    status.max_players = 4;
    status.players_connected = 2;
    status.players_ready = 1;
    status.ready_mask = 0x01;
    status.game_started = 0;
    
    EXPECT_EQ(status.max_players, 4);
    EXPECT_EQ(status.players_connected, 2);
    EXPECT_EQ(status.players_ready, 1);
    EXPECT_EQ(status.ready_mask, 0x01);
    EXPECT_EQ(status.game_started, 0);
}

TEST(ProtocolTest, ScoreUpdate) {
    ScoreUpdate score;
    score.client_id = 1;
    score.score = 1000;
    score.enemies_killed = 10;
    
    EXPECT_EQ(score.client_id, 1);
    EXPECT_EQ(score.score, 1000);
    EXPECT_EQ(score.enemies_killed, 10);
}

TEST(ProtocolTest, PacketSerialization) {
    ConnectRequest request;
    request.header.sequence_number = 42;
    std::strncpy(request.client_version, "1.0.0", sizeof(request.client_version) - 1);
    
    uint8_t buffer[sizeof(ConnectRequest)];
    std::memcpy(buffer, &request, sizeof(ConnectRequest));
    
    ConnectRequest deserialized;
    std::memcpy(&deserialized, buffer, sizeof(ConnectRequest));
    
    EXPECT_EQ(deserialized.header.magic, request.header.magic);
    EXPECT_EQ(deserialized.header.sequence_number, 42);
    EXPECT_STREQ(deserialized.client_version, "1.0.0");
}