#pragma once
class World;

struct User {
    int sessionIndex = -1;
    World* room = nullptr;
    int seat = -1;
    bool inUse = false;
};

User g_users[1000];