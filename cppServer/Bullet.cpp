#include"Bullet.h"

void Bullet::Fire(float startX, float startY, float dx, float dy) {
    active = true;
    x = startX;
    y = startY;
    dirX = dx;
    dirY = dy;
}

void Bullet::Update(float dt) {
    if (!active) return;

    x += dirX * speed * dt;
    y += dirY * speed * dt;
}

void Bullet::Clear() {
    active = false;
}

bool Bullet::IsActive() const { return active; }
float Bullet::GetX() const { return x; }
float Bullet::GetY() const { return y; }