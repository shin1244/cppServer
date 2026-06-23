#include"GameRoom.h"


void GameRoom::UpdatePlayers(float dt) {
    for (int i = 0; i < 1000; i++) {
        if (!playerList[i].IsActive()) continue;
        playerList[i].Update(dt);
    }
}

void GameRoom::UpdateBullets(float dt) {
    for (int i = 0; i < 1000; i++) {
        if (!bulletList[i].IsActive()) continue;
        bulletList[i].Update(dt);
    }
}

void GameRoom::CheckBulletHits() {
    for (int i = 0; i < 1000; i++) {
        if (!playerList[i].IsActive()) continue;

        for (int j = 0; j < 1000; j++) {
            if (!bulletList[j].IsActive()) continue;
            if (i == bulletList[j].GetOwnerId()) continue;

            float dx = playerList[i].GetX() - bulletList[j].GetX();
            float dy = playerList[i].GetY() - bulletList[j].GetY();

            float hitRadius = 14.0f;
            float distSq = dx * dx + dy * dy;

            if (distSq <= hitRadius * hitRadius) {
                std::cout << "KILL!!" << "\n";
                bulletList[j].Clear();
            }
        }
    }
}

void GameRoom::BroadcastState() {
    for (int i = 0; i < 1000; i++) {
        if (!playerList[i].IsActive()) continue;
        MovePacket mp;
        mp.h.size = sizeof(MovePacket);
        mp.h.id = static_cast<unsigned short>(PacketId::Move);
        mp.playerId = i;
        mp.x = playerList[i].GetX();
        mp.y = playerList[i].GetY();
        broadcast((const char*)&mp, sizeof(mp));
    }

    for (int i = 0; i < 1000; i++) {
        if (!bulletList[i].IsActive()) continue;
        BulletMovePacket bp;
        bp.h.size = sizeof(BulletMovePacket);
        bp.h.id = static_cast<unsigned short>(PacketId::Attack);
        bp.bulletId = i;
        bp.ownerId = bulletList[i].GetOwnerId();
        bp.x = bulletList[i].GetX();
        bp.y = bulletList[i].GetY();
        broadcast((const char*)&bp, sizeof(bp));
    }
}