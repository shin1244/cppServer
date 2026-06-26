#include"Player.h"

void Player::Init(int sessionIdx) {
    sessionIndex = sessionIdx;
    x = (homeMinX + homeMaxX) * 0.5f;
    y = (homeMinY + homeMaxY) * 0.5f;
    active = true;
}

void Player::Clear() {
    active = false;
}

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

    if (x < homeMinX) x = homeMinX;
    if (x > homeMaxX) x = homeMaxX;
    if (y < homeMinY) y = homeMinY;
    if (y > homeMaxY) y = homeMaxY;
}

void Player::Move(float dx, float dy) {
    x += dx;
    y += dy;
}

void Player::SetPos(float nx, float ny) { 
    x = nx; 
    y = ny; 
}

void Player::SetHome(float minX, float minY, float maxX, float maxY) {
    homeMinX = minX; homeMinY = minY;
    homeMaxX = maxX; homeMaxY = maxY;
}

bool Player::IsActive() { return active; }

float Player::GetX() const { return x; }
float Player::GetY() const { return y; }

float Player::GetHomeMinX() const { return homeMinX; }
float Player::GetHomeMinY() const { return homeMinY; }
float Player::GetHomeMaxX() const { return homeMaxX; }
float Player::GetHomeMaxY() const { return homeMaxY; }