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
    Destroy,
    Observe,   // 관전자에게 따라갈 대상 슬롯 id 를 알려준다 (IdPacket)
};

const int HEADER_SIZE = 4;

struct RecvPacket {
    int sessionIndex;
    PacketId id;
    std::vector<char> body;
};