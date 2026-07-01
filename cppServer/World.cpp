#include"World.h"

void World::Init() {
    map.Generate(W, Y, WALL, MAX_PLAYER, SEED);
    const auto& spawns = map.GetSpawnPoints();

    for (int i = 0; i < MAX_PLAYER; i++) {
        slots[i].player.SetPos(spawns[i].x, spawns[i].y);
    }
}

void World::Update(float dt) {
    UpdatePlayers(dt);
    UpdateBullets(dt);
    UpdateGrid();
    SendAOIUpdates();
}

void World::UpdatePlayers(float dt) {
    for (int i = 0; i < MAX_PLAYER; i++) {
        if (slots[i].state != SlotState::Playing) continue;
        slots[i].player.Update(dt);
    }
}

void World::UpdateBullets(float dt) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].IsActive()) continue;
        bullets[i].Update(dt);
    }
}

void World::UpdateGrid() {
    playerGrid.Clear();
    for (int i = 0; i < MAX_PLAYER; i++) {
        if (slots[i].state != SlotState::Playing) continue;
        playerGrid.Add(i, slots[i].player.GetX(), slots[i].player.GetY());
    }
    bulletGrid.Clear();
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].IsActive()) continue;
        bulletGrid.Add(i, bullets[i].GetX(), bullets[i].GetY());
    }
}

void World::SendAOIUpdates() {
    for (int i = 0; i < MAX_PLAYER; ++i) {
        if (slots[i].state != SlotState::Playing) continue;
        std::vector<int> visiblePlayers;
        playerGrid.QueryNeighbors(slots[i].player.GetX(), slots[i].player.GetY(), visiblePlayers);

        for (int target : visiblePlayers) {
            PlayerMovePacket pp;
			pp.h.id = static_cast<unsigned short>(PacketId::Move);
            pp.h.size = sizeof(PlayerMovePacket);
            pp.playerId = target;
            pp.x = slots[target].player.GetX();
            pp.y = slots[target].player.GetY();

            SendTo(i, (char*)&pp, pp.h.size);
        }

        std::vector<int> visibleBullets;
        bulletGrid.QueryNeighbors(slots[i].player.GetX(), slots[i].player.GetY(), visibleBullets);

        for (int target : visibleBullets) {
            BulletMovePacket bp;
			bp.h.id = static_cast<unsigned short>(PacketId::Move);
            bp.bulletId = target;
            bp.x = bullets[target].GetX();
            bp.y = bullets[target].GetY();

            SendTo(i, (char*)&bp, bp.h.size);
        }
    }
}

int World::FindEmptySlot() {
    for (int i = 0; i < MAX_PLAYER; ++i) {
        if (slots[i].state == SlotState::Empty) return i;
    }
    return -1;
}

int World::FindSlotBySession(int sessionIndex) {
    for (int i = 0; i < MAX_PLAYER; ++i) {
        if (slots[i].sessionIndex == sessionIndex) return i;
    }
    return -1;
}

void World::RemovePlayer(int i) {
    RemovePlayerPacket rp;
    rp.h.size = sizeof(RemovePlayerPacket);
    rp.h.id = static_cast<unsigned short>(PacketId::RemovePlayer);
    rp.playerId = i;

    Broadcast((char*)&rp, rp.h.size);
}

void World::RemoveBullet(int i) {
    bullets[i].Clear();

    RemoveBulletPacket rp;
    rp.h.size = sizeof(rp);
    rp.h.id = (unsigned short)PacketId::RemoveBullet;
    rp.bulletId = i;

    Broadcast((char*)&rp, rp.h.size);
}