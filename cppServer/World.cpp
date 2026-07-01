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

void World::HandlePacket(RecvPacket& packet) {
    switch (packet.id)
    {
    case PacketId::Move:
        HandleMove(packet);
        break;
    case PacketId::Attack:
        HandleAttack(packet);
        break;
    case PacketId::Join:
		HandleJoin(packet);
        break;
    case PacketId::Disconnect:
        HandleDisconnect(packet);
        break;
    default:
        break;
    }
}

void World::HandleMove(RecvPacket& packet) {
    if (packet.body.size() < 1) return;
    unsigned char keys = static_cast<unsigned char>(packet.body[0]);
    int idx = FindSlotBySession(packet.sessionIndex);
    slots[idx].player.SetKeys(keys);
}

void World::HandleAttack(RecvPacket& packet) {
    if (packet.body.size() < sizeof(float) * 2) return;
    int idx = FindSlotBySession(packet.sessionIndex);
    if (slots[idx].state != SlotState::Playing) return;

    float dirX;
    float dirY;

    memcpy(&dirX, packet.body.data(), sizeof(float));
    memcpy(&dirY, packet.body.data() + sizeof(float), sizeof(float));

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].IsActive()) continue;

        bullets[i].Fire(
            slots[idx].player.GetX(),
            slots[idx].player.GetY(),
            dirX,
            dirY
        );
        break;
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
    int idx = FindSlotBySession(packet.sessionIndex);
    slots[idx].state = SlotState::Empty;
    slots[idx].sessionIndex = -1;

    RemovePlayer(idx);
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
        if (slots[i].state == SlotState::Empty) continue;
        int idx = slots[i].sessionIndex;
        Session* s = &g_sessionList[idx];

        s->sendLock.lock();
        s->sendBuffer.Write(packet, len);
        flushSend(s);
        s->sendLock.unlock();
    }
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