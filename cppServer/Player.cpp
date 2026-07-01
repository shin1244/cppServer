#include"Player.h"

void Player::SetKeys(unsigned char k) { keys = k; }

void Player::Update(float dt) {
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