#pragma once
#include <cmath>
class Player {
private:
    float x = 0, y = 0;
    unsigned char keys = 0;
    float PLAYER_SPEED = 300.0f;
    float fireCooldown = 0.0f;
    float FIRE_INTERVAL = 0.25f;

public:
	// 플레이어의 입력 키를 설정합니다.
    void SetKeys(unsigned char k);
	// 플레이어의 위치를 업데이트합니다.
    void Update(float dt);
	// 플레이어를 dx, dy만큼 이동시킵니다.
    void Move(float dx, float dy);
	// 플레이어의 위치를 nx, ny로 설정합니다.
    void SetPos(float nx, float ny);
	// 플레이어의 현재 x 좌표를 반환합니다.
    float GetX() const;
	// 플레이어의 현재 y 좌표를 반환합니다.
    float GetY() const;
    // 플레이어가 사격할 수 있는 상태인지 확인합니다.
    bool CanFire() const { return fireCooldown <= 0.0f; }
    void StartFireCooldown() { fireCooldown = FIRE_INTERVAL; }
    void GetItem(int type);
    float GetSpeed() const { return PLAYER_SPEED; }
    float GetFireInterval() const { return FIRE_INTERVAL; }
};