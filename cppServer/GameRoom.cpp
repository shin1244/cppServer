#include"GameRoom.h"

void GameRoom::Init() {
    playerList[0].SetHome(300, 0, 600, 300);
    playerList[1].SetHome(300, 600, 600, 900);
    playerList[2].SetHome(0, 300, 300, 600);
    playerList[3].SetHome(600, 300, 900, 600);
}

void GameRoom::Update(float dt) {
    UpdatePlayers(dt);
    UpdateBullets(dt);
    CheckBulletHits();
    for (int i = 0; i < MAX_SEATS; i++) {
        SendStateToPlayer(i);
        SendStateToObserver(i);
    }

}

void GameRoom::UpdatePlayers(float dt) {
    for (int i = 0; i < MAX_SEATS; i++) {
        if (!playerList[i].IsActive()) continue;
        playerList[i].Update(dt);
    }
}

void GameRoom::UpdateBullets(float dt) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bulletList[i].IsActive()) continue;
        bulletList[i].Update(dt);
        if (bulletList[i].GetX() > 900 || bulletList[i].GetX() < 0
            || bulletList[i].GetY() > 900 || bulletList[i].GetY() < 0) {
            RemoveBullet(i);
        }
    }
}

void GameRoom::CheckBulletHits() {
    for (int i = 0; i < MAX_SEATS; i++) {
        if (!playerList[i].IsActive()) continue;

        for (int j = 0; j < MAX_BULLETS; j++) {
            if (!bulletList[j].IsActive()) continue;
            if (i == bulletList[j].GetOwnerId()) continue;

            float dx = playerList[i].GetX() - bulletList[j].GetX();
            float dy = playerList[i].GetY() - bulletList[j].GetY();

            float hitRadius = 14.0f;
            float distSq = dx * dx + dy * dy;

            if (distSq <= hitRadius * hitRadius) {
                RemovePlayer(i);
                RemoveBullet(j);
            }
        }
    }
}

void GameRoom::HandleConnect(RecvPacket& packet) {
    int seat = FindEmptySeat();
    if (seat == -1) return;

    int idx = packet.sessionIndex;
    seats[seat] = idx;
    playerList[seat].Init(idx);

    ConnectPacket cp;
    cp.h.size = sizeof(ConnectPacket);
    cp.h.id = static_cast<unsigned short>(PacketId::Connect);
    cp.playerId = seat;

    Broadcast((const char*)&cp, sizeof(cp));
}

void GameRoom::HandleDisconnect(RecvPacket& packet) {
    int idx = packet.sessionIndex;

    int seat = FindSeatBySession(idx);
    if (seat == -1) return;

    seats[seat] = -1;
    RemovePlayer(idx);
}

void GameRoom::HandleMove(RecvPacket& packet) {
    if (packet.body.size() < 1) return;
    unsigned char keys = static_cast<unsigned char>(packet.body[0]);
    int seat = FindSeatBySession(packet.sessionIndex);
    playerList[seat].SetKeys(keys);
}

void GameRoom::HandleAttack(RecvPacket& packet) {
    if (packet.body.size() < sizeof(float) * 2) return;
    int seat = FindSeatBySession(packet.sessionIndex);
    if (!playerList[seat].IsActive()) return;

    float dirX;
    float dirY;

    memcpy(&dirX, packet.body.data(), sizeof(float));
    memcpy(&dirY, packet.body.data() + sizeof(float), sizeof(float));

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bulletList[i].IsActive()) continue;

        bulletList[i].Fire(
            seat,
            playerList[seat].GetX(),
            playerList[seat].GetY(),
            dirX,
            dirY
        );

        std::cout << "Bullet fired owner=" << seat
            << " bullet=" << i
            << " dir=(" << dirX << ", " << dirY << ")\n";
        break;
    }
}

void GameRoom::HandlePacket(RecvPacket& packet) {
    switch (packet.id)
    {
    case PacketId::Move:
        HandleMove(packet);
        break;
    case PacketId::Connect:
        HandleConnect(packet);
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

int GameRoom::FindEmptySeat() {
    for (int i = 0; i < MAX_SEATS; ++i) {
        if (seats[i] == -1) return i;
    }
    return -1;
}

int GameRoom::FindSeatBySession(int sessionIndex) {
    for (int i = 0; i < MAX_SEATS; ++i) {
        if (seats[i] == sessionIndex) return i;
    }
    return -1;
}

void GameRoom::Broadcast(const char* packet, int len) {
    for (int i = 0; i < MAX_SEATS; ++i) {
        if (seats[i] == -1) continue;

        int idx = seats[i];
        Session* s = &g_sessionList[idx];

        s->sendLock.lock();
        s->sendBuffer.Write(packet, len);
        flushSend(s);
        s->sendLock.unlock();
    }
}

void GameRoom::RemovePlayer(int i) {
    playerList[i].Clear();

    RemovePlayerPacket rp;
    rp.h.size = sizeof(RemovePlayerPacket);
    rp.h.id = static_cast<unsigned short>(PacketId::RemovePlayer);
    rp.playerId = i;

    Broadcast((char*)&rp, rp.h.size);
}

void GameRoom::RemoveBullet(int i) {
    bulletList[i].Clear();

    RemoveBulletPacket rp;
    rp.h.size = sizeof(rp);
    rp.h.id = (unsigned short)PacketId::RemoveBullet;
    rp.bulletId = i;

    Broadcast((char*)&rp, rp.h.size);
}

bool GameRoom::IsVisible(int receiverSeat, float targetX, float targetY) {
    if (playerList[receiverSeat].IsInHomeZone(targetX, targetY))
        return true;

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bulletList[i].IsActive()) continue;
		if (bulletList[i].GetOwnerId() != receiverSeat) continue;
        float dx = targetX - bulletList[i].GetX();
        float dy = targetY - bulletList[i].GetY();
        if (abs(dx) < 100 && abs(dy) < 100)
            return true;
    }
    return false;
}

void GameRoom::SendStateToPlayer(int receiverSeat) {
    if (!playerList[receiverSeat].IsActive()) return;
    int idx = seats[receiverSeat];
    Session* s = &g_sessionList[idx];

    for (int i = 0; i < MAX_SEATS; i++) {
        if (!playerList[i].IsActive()) continue;
        if (IsVisible(receiverSeat, playerList[i].GetX(), playerList[i].GetY())) {
            MovePacket mp;

            mp.h.size = sizeof(MovePacket);
            mp.h.id = static_cast<unsigned short>(PacketId::Move);
            mp.playerId = i;
            mp.x = playerList[i].GetX();
            mp.y = playerList[i].GetY();

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
        if (!bulletList[i].IsActive()) continue;
        if (IsVisible(receiverSeat, bulletList[i].GetX(), bulletList[i].GetY())) {

            BulletMovePacket bp;
            bp.h.size = sizeof(BulletMovePacket);
            bp.h.id = static_cast<unsigned short>(PacketId::Attack);
            bp.bulletId = i;
            bp.ownerId = bulletList[i].GetOwnerId();
            bp.x = bulletList[i].GetX();
            bp.y = bulletList[i].GetY();

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

void GameRoom::SendStateToObserver(int receiverSeat) {
    if (playerList[receiverSeat].IsActive() || seats[receiverSeat] == -1) return;
    int idx = seats[receiverSeat];
    Session* s = &g_sessionList[idx];

    for (int i = 0; i < MAX_SEATS; i++) {
        if (!playerList[i].IsActive()) continue;

        MovePacket mp;

        mp.h.size = sizeof(MovePacket);
        mp.h.id = static_cast<unsigned short>(PacketId::Move);
        mp.playerId = i;
        mp.x = playerList[i].GetX();
        mp.y = playerList[i].GetY();

        s->sendLock.lock();
        s->sendBuffer.Write((const char*)&mp, mp.h.size);
        flushSend(s);
        s->sendLock.unlock();
    }

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bulletList[i].IsActive()) continue;

            BulletMovePacket bp;
            bp.h.size = sizeof(BulletMovePacket);
            bp.h.id = static_cast<unsigned short>(PacketId::Attack);
            bp.bulletId = i;
            bp.ownerId = bulletList[i].GetOwnerId();
            bp.x = bulletList[i].GetX();
            bp.y = bulletList[i].GetY();

            s->sendLock.lock();
            s->sendBuffer.Write((const char*)&bp, bp.h.size);
            flushSend(s);
            s->sendLock.unlock();
        }
    }