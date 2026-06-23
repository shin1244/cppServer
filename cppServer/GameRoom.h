#pragma once
#include"Player.h"
#include"Bullet.h"
#include<iostream>
#include"Protocol.h"

class GameRoom {
private:
    Player playerList[1000];
    Bullet bulletList[1000];
public:
	void Update(float dt);
private:
    void UpdatePlayers(float dt);
    void UpdateBullets(float dt);
    void CheckBulletHits();
    void CheckTerritory();
    void BroadcastState();
};