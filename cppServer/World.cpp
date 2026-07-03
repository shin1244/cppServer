#include"World.h"

void World::Init() {
    running = false;
    map.Generate(W, Y, WALL, MAX_PLAYER, SEED);
    const auto& spawns = map.GetSpawnPoints();

    float ww = map.GetWorldWidth();
    float wh = map.GetWorldHeight();
    playerGrid.Init(ww, wh, 280);
    bulletGrid.Init(ww, wh, 280);

    for (int i = 0; i < MAX_PLAYER; i++) {
        slots[i].player.SetPos(spawns[i].x, spawns[i].y);
    }
}

void World::Update(float dt) {
    if (!running) return;
    UpdatePlayers(dt);
    UpdateBullets(dt);
    UpdateGrid();
    Collision();
    SendAOIUpdates();
}

void World::UpdatePlayers(float dt) {
    for (int i = 0; i < MAX_PLAYER; i++) {
        if (slots[i].state != SlotState::Playing) continue;
        float ox = slots[i].player.GetX();
        float oy = slots[i].player.GetY();

        slots[i].player.Update(dt);

        float cx = slots[i].player.GetX();
        float cy = slots[i].player.GetY();
        if (map.IsWall(cx, cy)) {
            slots[i].player.SetPos(ox, oy);
        }
    }
}

void World::UpdateBullets(float dt) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].IsActive()) continue;
        bullets[i].Update(dt);

        float cx = bullets[i].GetX();
        float cy = bullets[i].GetY();
        if (map.IsWall(cx, cy)) {
            RemoveBullet(i);
            if (map.DamageWall(cx, cy)) {
                Vec2Packet p;
                p.h.id = static_cast<unsigned short>(PacketId::Destroy);
                p.h.size = sizeof(Vec2Packet);
                p.x = cx;
                p.y = cy;

                Broadcast((char*)&p, p.h.size);
            }
        }
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

void World::Collision() {
    for (int i = 0; i < MAX_PLAYER; ++i) {
        if (slots[i].state != SlotState::Playing) continue;
        float px = slots[i].player.GetX();
        float py = slots[i].player.GetY();

        std::vector<int> nearBullets;
        bulletGrid.QueryNeighbors(
            px,
            py, 
            nearBullets
        );
        for (int b : nearBullets) {
            if (!bullets[b].IsActive()) continue;
            float dx = px - bullets[b].GetX();
            float dy = py - bullets[b].GetY();
            float r = PLAYER_RADIUS + BULLET_RADIUS;
            if (dx * dx + dy * dy <= r * r) {
                slots[i].state = SlotState::Observing;
                RemovePlayer(i);
                RemoveBullet(b);
                std::cout << "Kill!" << i << "\n";
            }
        }
    }
}

void World::SendAOIUpdates() {
    for (int i = 0; i < MAX_PLAYER; ++i) {
        if (slots[i].state != SlotState::Playing) continue;
        std::vector<int> visiblePlayers;
        playerGrid.QueryNeighbors(slots[i].player.GetX(), slots[i].player.GetY(), visiblePlayers);

        for (int target : visiblePlayers) {
            if (slots[target].state != SlotState::Playing) continue;
            Vec2Packet p;
			p.h.id = static_cast<unsigned short>(PacketId::MovePlayer);
            p.h.size = sizeof(Vec2Packet);
            p.id = target;
            p.x = slots[target].player.GetX();
            p.y = slots[target].player.GetY();

            SendTo(i, (char*)&p, p.h.size);
        }

        std::vector<int> visibleBullets;
        bulletGrid.QueryNeighbors(slots[i].player.GetX(), slots[i].player.GetY(), visibleBullets);

        for (int target : visibleBullets) {
            if (!bullets[target].IsActive()) continue;

            Vec2Packet p;
			p.h.id = static_cast<unsigned short>(PacketId::MoveBullet);
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