#pragma once
#include <cmath>
class Player {
private:
    int sessionIndex = -1;
    float x = 0, y = 0;
    bool active = false;
    unsigned char keys = 0;
    float PLAYER_SPEED = 300.0f;

    float homeMinX = 0, homeMinY = 0;
    float homeMaxX = 0, homeMaxY = 0;

public:
    void Init(int sessionIdx) {
        sessionIndex = sessionIdx;
        x = (homeMinX + homeMaxX) * 0.5f;
        y = (homeMinY + homeMaxY) * 0.5f;
        active = true;
    }

    void Clear() {
        active = false;
    }

    void SetKeys(unsigned char k) { keys = k; }

    void Update(float dt) {
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

    void Move(float dx, float dy) {
        x += dx;
        y += dy;
    }

    void SetPos(float nx, float ny) { x = nx; y = ny; }

    void SetHome(float minX, float minY, float maxX, float maxY) {
        homeMinX = minX; homeMinY = minY;
        homeMaxX = maxX; homeMaxY = maxY;
    }

    bool IsActive() { return active; }

    float GetX() const { return x; }
    float GetY() const { return y; }

    float GetHomeMinX() const { return homeMinX; }
    float GetHomeMinY() const { return homeMinY; }
    float GetHomeMaxX() const { return homeMaxX; }
    float GetHomeMaxY() const { return homeMaxY; }
};