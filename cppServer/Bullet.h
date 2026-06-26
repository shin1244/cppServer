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
    void Fire(int owner, float startX, float startY, float dx, float dy);
    void Update(float dt);
    void Clear();
    bool IsActive() const;
    int GetOwnerId() const;
    float GetX() const;
    float GetY() const;
};