#include"World.h"

void World::Init() {
    map.Generate(W, Y, WALL, MAX_PLAYER, SEED);
    const auto& spawns = map.GetSpawnPoints();

    for (int i = 0; i < MAX_PLAYER; i++) {
        players[i].SetPos(spawns[i].x, spawns[i].y);
    }
}

void World::Update(float dt) {
    UpdatePlayers(dt);
    UpdateBullets(dt);
    CheckBulletHits();
    for (int i = 0; i < MAX_PLAYER; i++) {
        SendStateToPlayer(i);
    }
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

void World::CheckBulletHits() {
    for (int i = 0; i < MAX_PLAYER; i++) {
        if (!players[i].IsActive()) continue;

        for (int j = 0; j < MAX_BULLETS; j++) {
            if (!bullets[j].IsActive()) continue;
            if (i == bullets[j].GetOwnerId()) continue;

            float dx = players[i].GetX() - bullets[j].GetX();
            float dy = players[i].GetY() - bullets[j].GetY();

            float hitRadius = 14.0f;
            float distSq = dx * dx + dy * dy;

            if (distSq <= hitRadius * hitRadius) {
                RemovePlayer(i);
                RemoveBullet(j);
            }
        }
    }
}

void World::HandleJoin(RecvPacket& packet) {
    int seat = FindEmptySlot();
    if (seat == -1) return;

    int idx = packet.sessionIndex;

    ConnectPacket cp;
    cp.h.size = sizeof(ConnectPacket);
    cp.h.id = static_cast<unsigned short>(PacketId::Connect);
    cp.playerId = seat;

    Broadcast((const char*)&cp, sizeof(cp));
}

void World::HandleDisconnect(RecvPacket& packet) {
    int idx = packet.sessionIndex;

    int seat = FindSlotBySession(idx);
    if (seat == -1) return;

    seats[seat] = -1;
    RemovePlayer(idx);
}

void World::HandleMove(RecvPacket& packet) {
    if (packet.body.size() < 1) return;
    unsigned char keys = static_cast<unsigned char>(packet.body[0]);
    int seat = FindSlotBySession(packet.sessionIndex);
    players[seat].SetKeys(keys);
}

void World::HandleAttack(RecvPacket& packet) {
    if (packet.body.size() < sizeof(float) * 2) return;
    int seat = FindSlotBySession(packet.sessionIndex);
    if (!players[seat].IsActive()) return;

    float dirX;
    float dirY;

    memcpy(&dirX, packet.body.data(), sizeof(float));
    memcpy(&dirY, packet.body.data() + sizeof(float), sizeof(float));

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].IsActive()) continue;

        bullets[i].Fire(
            seat,
            players[seat].GetX(),
            players[seat].GetY(),
            dirX,
            dirY
        );

        std::cout << "Bullet fired owner=" << seat
            << " bullet=" << i
            << " dir=(" << dirX << ", " << dirY << ")\n";
        break;
    }
}

void World::HandlePacket(RecvPacket& packet) {
    switch (packet.id)
    {
    case PacketId::Move:
        HandleMove(packet);
        break;
    case PacketId::Join:
		HandleJoin(packet);
        break;
    case PacketId::Disconnect:
        HandleDisconnect(packet);
        break;
    case PacketId::Attack:
        HandleAttack(packet);
        break;
    default:
        break;
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

void World::Broadcast(const char* packet, int len) {
    for (int i = 0; i < MAX_PLAYER; ++i) {
        if (seats[i] == -1) continue;

        int idx = seats[i];
        Session* s = &g_sessionList[idx];

        s->sendLock.lock();
        s->sendBuffer.Write(packet, len);
        flushSend(s);
        s->sendLock.unlock();
    }
}

void World::RemovePlayer(int i) {
    players[i].Clear();

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

bool World::IsVisible(int receiverSeat, float targetX, float targetY) {
    if (players[receiverSeat].IsInHomeZone(targetX, targetY))
        return true;

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].IsActive()) continue;
		if (bullets[i].GetOwnerId() != receiverSeat) continue;
        float dx = targetX - bullets[i].GetX();
        float dy = targetY - bullets[i].GetY();
        if (abs(dx) < 100 && abs(dy) < 100)
            return true;
    }
    return false;
}

void World::SendStateToPlayer(int receiverSeat) {
    if (!players[receiverSeat].IsActive()) return;
    int idx = seats[receiverSeat];
    Session* s = &g_sessionList[idx];

    for (int i = 0; i < MAX_PLAYER; i++) {
        if (!players[i].IsActive()) continue;
        if (IsVisible(receiverSeat, players[i].GetX(), players[i].GetY())) {
            MovePacket mp;

            mp.h.size = sizeof(MovePacket);
            mp.h.id = static_cast<unsigned short>(PacketId::Move);
            mp.playerId = i;
            mp.x = players[i].GetX();
            mp.y = players[i].GetY();

            s->sendLock.lock();
            s->sendBuffer.Write((const char*)&mp, mp.h.size);
            flushSend(s);
            s->sendLock.unlock();
        }
        else {
            HidePlayerPacket hpp;
            hpp.h.size = sizeof(HidePlayerPacket);
            hpp.h.id = static_cast<unsigned short>(PacketId::HidePlayer);
            hpp.playerId = i;

            s->sendLock.lock();
            s->sendBuffer.Write((const char*)&hpp, hpp.h.size);
            flushSend(s);
            s->sendLock.unlock();
        }
    }
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].IsActive()) continue;
        if (IsVisible(receiverSeat, bullets[i].GetX(), bullets[i].GetY())) {

            BulletMovePacket bp;
            bp.h.size = sizeof(BulletMovePacket);
            bp.h.id = static_cast<unsigned short>(PacketId::Attack);
            bp.bulletId = i;
            bp.ownerId = bullets[i].GetOwnerId();
            bp.x = bullets[i].GetX();
            bp.y = bullets[i].GetY();

            s->sendLock.lock();
            s->sendBuffer.Write((const char*)&bp, bp.h.size);
            flushSend(s);
            s->sendLock.unlock();
        }
        else {
            HideBulletPacket hbp;
            hbp.h.size = sizeof(HideBulletPacket);
            hbp.h.id = static_cast<unsigned short>(PacketId::HideBullet);
            hbp.bulletId = i;

            s->sendLock.lock();
            s->sendBuffer.Write((const char*)&hbp, hbp.h.size);
            flushSend(s);
            s->sendLock.unlock();
        }
    }
}