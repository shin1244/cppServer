#include"World.h"

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

    IdPacket p;
    p.h.size = sizeof(IdPacket);
    p.h.id = static_cast<unsigned short>(PacketId::Join);
    p.id = seat;

    Broadcast((const char*)&p, p.h.size);
}

void World::HandleDisconnect(RecvPacket& packet) {
    int idx = FindSlotBySession(packet.sessionIndex);
    slots[idx].state = SlotState::Empty;
    slots[idx].sessionIndex = -1;

    RemovePlayer(idx);
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

void World::SendTo(int idx, const char* packet, int len) {
    if (slots[idx].state == SlotState::Empty) return;
    int idx = slots[idx].sessionIndex;
    Session* s = &g_sessionList[idx];

    s->sendLock.lock();
    s->sendBuffer.Write(packet, len);
    flushSend(s);
    s->sendLock.unlock();
}