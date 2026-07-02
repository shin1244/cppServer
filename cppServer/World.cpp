#include"World.h"

void World::Init() {
    running = false;
    map.Generate(W, Y, WALL, MAX_PLAYER, SEED);
    const auto& spawns = map.GetSpawnPoints();

    float ww = map.GetWorldWidth();
    float wh = map.GetWorldHeight();
    playerGrid.Init(ww, wh, 100);
    bulletGrid.Init(ww, wh, 100);

    for (int i = 0; i < MAX_PLAYER; i++) {
        slots[i].player.SetPos(spawns[i].x, spawns[i].y);
    }
}

void World::Update(float dt) {
    if (!running) return;
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
            Vec2Packet p;
			p.h.id = static_cast<unsigned short>(PacketId::RemovePlayer);
            p.h.size = sizeof(Vec2Packet);
            p.id = target;
            p.x = slots[target].player.GetX();
            p.y = slots[target].player.GetY();

            SendTo(i, (char*)&p, p.h.size);
        }

        std::vector<int> visibleBullets;
        bulletGrid.QueryNeighbors(slots[i].player.GetX(), slots[i].player.GetY(), visibleBullets);

        for (int target : visibleBullets) {
            Vec2Packet p;
			p.h.id = static_cast<unsigned short>(PacketId::RemoveBullet);
            p.id = target;
            p.h.size = sizeof(Vec2Packet);
            p.x = bullets[target].GetX();
            p.y = bullets[target].GetY();

            SendTo(i, (char*)&p, p.h.size);
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
    IdPacket p;
    p.h.size = sizeof(IdPacket);
    p.h.id = static_cast<unsigned short>(PacketId::RemovePlayer);
    p.id = i;

    Broadcast((char*)&p, p.h.size);
}

void World::RemoveBullet(int i) {
    bullets[i].Clear();

    IdPacket p;
    p.h.size = sizeof(p);
    p.h.id = (unsigned short)PacketId::RemoveBullet;
    p.id = i;

    Broadcast((char*)&p, p.h.size);
}