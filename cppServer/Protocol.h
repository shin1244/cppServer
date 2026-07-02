#pragma once
#include<vector>

#pragma pack(push, 1)
struct PacketHeader { unsigned short size; unsigned short id; };

struct WallPos {
    unsigned short x, y;
};

struct MapSnapHeader {
    PacketHeader h;
    unsigned short cellSize;
    unsigned short width;
    unsigned short height;
    unsigned short wallCount;
};

struct IdPacket {
    PacketHeader h;
    int id;
};

struct Vec2Packet {
    PacketHeader h;
    int id;
    float x;
    float y;
};
#pragma pack(pop)

enum class PacketId : unsigned short {
    Connect,
    Disconnect,
    Join,
    Leave,
    MovePlayer,
    MoveBullet,
    Attack,
    RemovePlayer,
    RemoveBullet,
    HidePlayer,
    HideBullet,
    Chat,
    MapSnapshot,
};

const int HEADER_SIZE = 4;

struct RecvPacket {
    int sessionIndex;
    PacketId id;
    std::vector<char> body;
};