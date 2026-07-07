#pragma once
#include"Player.h"
#include"Bullet.h"
#include<iostream>
#include"Protocol.h"
#include"NetworkCore.h"
#include"ObjectPool.h"
#include"Map.h"
#include"SpatialGrid.h"

constexpr int MAX_PLAYER = 200;
constexpr int MAX_BULLETS = 4096;

//#define USE_NOT_GRID

enum class SlotState {
    Empty,
    Waiting,
    Playing,
    Observing,
};

struct PlayerSlot
{
    int sessionIndex = -1;
    SlotState state = SlotState::Empty;

    std::vector<int> observers;
    int target = -1;

    Player player;
};

class World {
public:
    void Init();
    void Update(float dt);
    void HandlePacket(RecvPacket& packet); 

    // -- 벤치마크를 위한 접근자 --
    bool IsRunning() { return running; }
    bool CanJoin() const;

private:
    static const int W = 100;
    static const int Y = 100;
    static const int WALL = 700;
    static const int SEED = 1234;
    static constexpr float PLAYER_RADIUS = 10.0f;
    static constexpr float BULLET_RADIUS = 4.0f;

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
    void HandleObserve(RecvPacket&);

    // - UPDATE FLOW -
    void UpdatePlayers(float dt);
    void UpdateBullets(float dt);
    void UpdateGrid();
    void Collision();
    void CheckMatchEnd();
    void SendAOIUpdates();

    void RemovePlayer(int i);
    void RemoveBullet(int i);
    void Broadcast(const char* packet, int len);
    void SendTo(int idx, const char* packet, int len);
    void BroadcastMapSnapshot();

    int FindEmptySlot();
    int FindNextPlayingSlot(int idx);
    int FindSlotBySession(int sessionIndex);
    void TryStartMatch();
    void ObservePlayer(int observerIdx, int targetIdx);
};