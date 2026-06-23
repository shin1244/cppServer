#pragma once
class Player {
private:
    int sessionIndex = -1;
    float x = 0, y = 0;
    bool active = false;
    unsigned char keys = 0;
    float PLAYER_SPEED = 300.0f;

public:
    void Init(int sessionIdx) {
        sessionIndex = sessionIdx;
        x = 0;
        y = 0;
        active = true;
    }

    void Clear() {
        active = false;
    }

    void SetKeys(unsigned char k) { keys = k; }

    void Update(float dt) {
        float dx = 0, dy = 0;
        if (keys & 0x01) dy -= 1;   // W
        if (keys & 0x02) dy += 1;   // S
        if (keys & 0x04) dx -= 1;   // A
        if (keys & 0x08) dx += 1;   // D

        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0) {
            dx /= len; dy /= len; 
            x += dx * PLAYER_SPEED * dt;
            y += dy * PLAYER_SPEED * dt;
        }
    }

    void Move(float dx, float dy) {
        x += dx;
        y += dy;
    }

    bool IsActive() { return active; }

    float GetX() const { return x; }
    float GetY() const { return y; }
};