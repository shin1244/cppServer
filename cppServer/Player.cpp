#include"Player.h"

void Player::SetKeys(unsigned char k) { keys = k; }

void Player::Update(float dt) {
    if (fireCooldown > 0.0f) fireCooldown -= dt;

    float dx = 0, dy = 0;
    if (keys & 0x01) dy -= 1;
    if (keys & 0x02) dy += 1;
    if (keys & 0x04) dx -= 1;
    if (keys & 0x08) dx += 1;

    float len = std::sqrt(dx * dx + dy * dy);
    if (len > 0) {
        dx /= len; dy /= len;
        x += dx * PLAYER_SPEED * dt;
        y += dy * PLAYER_SPEED * dt;
    }
}

void Player::Move(float dx, float dy) {
    x += dx;
    y += dy;
}

void Player::SetPos(float nx, float ny) { 
    x = nx; 
    y = ny; 
}

float Player::GetX() const { return x; }
float Player::GetY() const { return y; }
void Player::GetItem(int type) {
    switch (type)
    {
    case 1:
        // 사격 간격을 줄여 사격 속도를 올린다. 하한 0.05초(=20발/초)까지.
        if (FIRE_INTERVAL > 0.05f) FIRE_INTERVAL -= 0.05f;
        break;
    case 2:
        if (PLAYER_SPEED < 450.0f) PLAYER_SPEED += 10.0f;
        break;
    default:
        break;
    }
}