#pragma once

class Bullet {
private:
    bool active = false;
    int ownerId = -1;
    float x = 0;
    float y = 0;
    float dirX = 0;
    float dirY = 0;
    float speed = 700.0f;

public:
    void Fire(int owner, float startX, float startY, float dx, float dy) {
        active = true;
        ownerId = owner;
        x = startX;
        y = startY;
        dirX = dx;
        dirY = dy;
    }

    void Update(float dt) {
        if (!active) return;

        x += dirX * speed * dt;
        y += dirY * speed * dt;
    }

    void Clear() {
        active = false;
        ownerId = -1;
    }

    bool IsActive() const { return active; }
    int GetOwnerId() const { return ownerId; }
    float GetX() const { return x; }
    float GetY() const { return y; }
};