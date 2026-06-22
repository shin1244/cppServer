#pragma once

class Player {
private:
    int sessionIndex = -1;
    float x = 0, y = 0;
    bool active = false;

public:
    void Init(int sessionIdx) {
        sessionIndex = sessionIdx;
        x = 0;
        y = 0;
        active = true;
    }

    void Move(float dx, float dy) {
        x += dx;
        y += dy;
    }

    bool IsActive() { return active; }

    //float GetX() const { return x; }
    //float GetY() const { return y; }
};