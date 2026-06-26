#include"Bullet.h"

void Bullet::Fire(int owner, float startX, float startY, float dx, float dy) {
    active = true;
    ownerId = owner;
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
    ownerId = -1;
}

bool Bullet::IsActive() const { return active; }
int Bullet::GetOwnerId() const { return ownerId; }
float Bullet::GetX() const { return x; }
float Bullet::GetY() const { return y; }