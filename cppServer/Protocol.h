#pragma once

#pragma pack(push, 1)
struct PacketHeader { unsigned short size; unsigned short id; };

struct MovePacket { PacketHeader h; int playerId; float x, y; };
struct AttackPacket { PacketHeader h; int playerId; float dirX, dirY; int bulletId; };
struct ConnectPacket { PacketHeader h; int playerId; };
#pragma pack(pop)

enum class PacketId : unsigned short {
    Connect,
    Disconnect,
    Move,
    Attack,
    Chat,
};

const int HEADER_SIZE = 4;

struct RecvPacket {
    int sessionIndex;
    PacketId id;
    std::vector<char> body;
};