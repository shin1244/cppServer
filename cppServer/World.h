#pragma once
#include"Player.h"
#include"Bullet.h"
#include<iostream>
#include"Protocol.h"
#include"NetworkCore.h"
#include"ObjectPool.h"
#include"Map.h"
#include"SpatialGrid.h"

enum class SlotState {
    Empty,
    Waiting,
    Playing,
    Observing,
};

struct PlayerSlot {
    int sessionIndex = -1;
    SlotState state = SlotState::Empty;

    Player player;
};

class World {
public:
    void Init();
    void Update(float dt);
    void HandlePacket(RecvPacket& packet); 

private:
    static const int W = 20;
    static const int Y = 20;
    static const int WALL = 100;
    static const int SEED = 1234;
    static const int MAX_PLAYER = 4;
    static const int MAX_BULLETS = 1024;

    bool running;
    Map map;
    PlayerSlot slots[MAX_PLAYER];
    Bullet bullets[MAX_BULLETS];
    SpatialGrid playerGrid;
    SpatialGrid bulletGrid;

    void HandleJoin(RecvPacket&);
    void HandleLeave(RecvPacket&);
    void HandleMove(RecvPacket&);
    void HandleAttack(RecvPacket&);

    void UpdatePlayers(float dt);
    void UpdateBullets(float dt);
    void UpdateGrid();
    void SendAOIUpdates();

    void RemovePlayer(int i);
    void RemoveBullet(int i);
    void Broadcast(const char* packet, int len);
    void SendTo(int idx, const char* packet, int len);

    int FindEmptySlot();
    int FindSlotBySession(int sessionIndex);
    void TryStartMatch();
};