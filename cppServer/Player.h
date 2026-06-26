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
    void Init(int sessionIdx);

    void Clear();

    void SetKeys(unsigned char k);

    void Update(float dt);

    void Move(float dx, float dy);

    void SetPos(float nx, float ny);

    void SetHome(float minX, float minY, float maxX, float maxY);

    bool IsActive();

    float GetX() const;
    float GetY() const;

    float GetHomeMinX() const;
    float GetHomeMinY() const;
    float GetHomeMaxX() const;
    float GetHomeMaxY() const;
};