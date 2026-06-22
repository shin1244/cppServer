#pragma once

class Player {
private:
    int sessionIndex;
    float x, y;

public:
    Player(int sessionIdx) : sessionIndex(sessionIdx), x(0), y(0) {}

    void Move(float dx, float dy) {
        x += dx;
        y += dy;
    }

    float GetX() const { return x; }
    float GetY() const { return y; }
};