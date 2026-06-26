#pragma once
#include<vector>

#pragma pack(push, 1)
struct PacketHeader { unsigned short size; unsigned short id; };
struct MovePacket { PacketHeader h; int playerId; float x, y; };
struct BulletMovePacket { PacketHeader h; int bulletId; int ownerId; float x;float y; };
struct ConnectPacket { PacketHeader h; int playerId; };
struct DisconnectPacket { PacketHeader h; int playerId;  };
struct RemovePacket { PacketHeader h; int bulletId; };
struct HidePlayerPacket { PacketHeader h; int playerId; };
struct HideBulletPacket { PacketHeader h; int bulletId; };
#pragma pack(pop)

enum class PacketId : unsigned short {
    Connect,
    Disconnect,
    Move,
    Attack,
    Remove,
    HidePlayer,
    HideBullet,
    Chat,
};

const int HEADER_SIZE = 4;

struct RecvPacket {
    int sessionIndex;
    PacketId id;
    std::vector<char> body;
};