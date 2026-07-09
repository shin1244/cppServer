#pragma once
class Item {
private:
    bool active = false;
    float x = 0, y = 0;
    int type = 0;   // RollWallDropItemId()°¡ ¹ñ´Â 0~3
public:
    void Spawn(float sx, float sy, int t);
    void Clear();
    bool IsActive() const;
    float GetX() const;
    float GetY() const;
    int GetType() const;

    bool ContainsPoint(float px, float py, float radius) const;
};