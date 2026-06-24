#pragma once
class GameRoom;

struct User {
    int sessionIndex = -1;
    GameRoom* room = nullptr;
    int seat = -1;
    bool inUse = false;
};