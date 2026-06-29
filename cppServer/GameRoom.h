#pragma once
#include"Player.h"
#include"Bullet.h"
#include<iostream>
#include"Protocol.h"
#include"NetworkCore.h"

class GameRoom {
public:
    void Init();
    void Update(float dt);
    void HandlePacket(RecvPacket& packet); 

private:
    void HandleConnect(RecvPacket&);
    void HandleDisconnect(RecvPacket&);
    void HandleMove(RecvPacket&);
    void HandleAttack(RecvPacket&);

    void UpdatePlayers(float dt);
    void UpdateBullets(float dt);
    void CheckBulletHits();
    void BroadcastObserver();
    void RemovePlayer(int i);
    void RemoveBullet(int i);
    void Broadcast(const char* packet, int len);

    int FindEmptySeat();
    int FindSeatBySession(int sessionIndex);

    static const int MAX_SEATS = 4;
    static const int MAX_BULLETS = 64;

    Player playerList[MAX_SEATS];
    int seats[MAX_SEATS] = { -1, -1, -1, -1 };

    Bullet bulletList[64];

	bool IsVisible(int receiverSeat, float targetX, float targetY);
    void SendStateToPlayer(int receiverSeat);
};