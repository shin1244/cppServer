#include"World.h"
#include <algorithm>

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
    CheckMatchEnd();
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
        if (map.IsWall(cx, oy)) {
            cx = ox;
        }
        if (map.IsWall(cx, cy)) {
            cy = oy;
        }
        slots[i].player.SetPos(cx, cy);
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
                int target = FindNextPlayingSlot(i);

                slots[i].observers.clear();

                slots[i].target = -1;      // 피격 직전 Playing 상태였으니 관전 대상 없음
                ObservePlayer(i, target);  // 피격자를 관전자로 전환
                for (int ob : slots[i].observers) {
                    ObservePlayer(ob, target); // 피격자를 보던 관전자들도 새 대상으로
                }

                RemovePlayer(i);
                RemoveBullet(b);
                std::cout << "Kill!" << i << "\n";
            }
        }
    }
}

void World::CheckMatchEnd() {
    int alive = 0, winner = -1;
    for (int i = 0; i < MAX_PLAYER; ++i)
        if (slots[i].state == SlotState::Playing) { alive++; winner = i; }

    if (alive <= 1) {
        running = false;
        IdPacket p;
        p.h.id = static_cast<unsigned short>(PacketId::End);
        p.h.size = sizeof(IdPacket);
        p.id = winner;

        Broadcast((char*)&p, p.h.size);
    }
}

void World::SendAOIUpdates() {
    for (int i = 0; i < MAX_PLAYER; ++i) {
        if (slots[i].state != SlotState::Playing) continue;
        std::vector<int> visiblePlayers;
        playerGrid.QueryNeighbors(slots[i].player.GetX(), slots[i].player.GetY(), visiblePlayers);

        float myX = slots[i].player.GetX();
        float myY = slots[i].player.GetY();

        for (int target : visiblePlayers) {
            if (slots[target].state != SlotState::Playing) continue;
            if (target != i &&
                !map.HasLineOfSight(myX, myY,
                    slots[target].player.GetX(), slots[target].player.GetY()))
                continue;   // 벽에 가려짐
            Vec2Packet p;
			p.h.id = static_cast<unsigned short>(PacketId::MovePlayer);
            p.h.size = sizeof(Vec2Packet);
            p.id = target;
            p.x = slots[target].player.GetX();
            p.y = slots[target].player.GetY();

            SendTo(i, (char*)&p, p.h.size);
            for (int ob : slots[i].observers) SendTo(ob, (char*)&p, p.h.size);
        }

        std::vector<int> visibleBullets;
        bulletGrid.QueryNeighbors(slots[i].player.GetX(), slots[i].player.GetY(), visibleBullets);

        for (int target : visibleBullets) {
            if (!bullets[target].IsActive()) continue;
            if (!map.HasLineOfSight(myX, myY,
                    bullets[target].GetX(), bullets[target].GetY()))
                continue;   // 벽에 가려짐

            Vec2Packet p;
			p.h.id = static_cast<unsigned short>(PacketId::MoveBullet);
            p.id = target;
            p.h.size = sizeof(Vec2Packet);
            p.x = bullets[target].GetX();
            p.y = bullets[target].GetY();

            SendTo(i, (char*)&p, p.h.size);
            for (int ob : slots[i].observers) SendTo(ob, (char*)&p, p.h.size);
        }
    }
}

int World::FindEmptySlot() {
    for (int i = 0; i < MAX_PLAYER; ++i) {
        if (slots[i].state == SlotState::Empty) return i;
    }
    return -1;
}

int World::FindNextPlayingSlot(int idx) {
    for (int i = 1; i < MAX_PLAYER; ++i) {
        int next_idx = (idx + i) % MAX_PLAYER;
        if (slots[next_idx].state == SlotState::Playing) {
            return next_idx;
        }
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

void World::ObservePlayer(int observerIdx, int targetIdx) {
    if (targetIdx < 0) return;

    int prev = slots[observerIdx].target;
    if (prev >= 0) {
        auto& obs = slots[prev].observers;
        obs.erase(std::remove(obs.begin(), obs.end(), observerIdx), obs.end());
    }

    slots[targetIdx].observers.push_back(observerIdx);
    slots[observerIdx].target = targetIdx;

    IdPacket sp;
    sp.h.size = sizeof(IdPacket);
    sp.h.id = static_cast<unsigned short>(PacketId::Observe);
    sp.id = targetIdx;

    SendTo(observerIdx, (char*)&sp, sp.h.size);
}