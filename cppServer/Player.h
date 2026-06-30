#pragma once
#include <cmath>
class Player {
private:
    int sessionIndex = -1;
    float x = 0, y = 0;
    unsigned char keys = 0;
    float PLAYER_SPEED = 300.0f;

    float homeMinX = 0, homeMinY = 0;
    float homeMaxX = 0, homeMaxY = 0;

public:
	// 세션 인덱스를 받아 플레이어를 초기화합니다.
    void Init(int sessionIdx);
	// 플레이어의 입력 키를 설정합니다.
    void SetKeys(unsigned char k);
	// 플레이어의 위치를 업데이트합니다.
    void Update(float dt);
	// 플레이어를 dx, dy만큼 이동시킵니다.
    void Move(float dx, float dy);
	// 플레이어의 위치를 nx, ny로 설정합니다.
    void SetPos(float nx, float ny);
	// 플레이어의 홈 영역을 설정합니다.
    void SetHome(float minX, float minY, float maxX, float maxY);
	// 플레이어의 현재 x 좌표를 반환합니다.
    float GetX() const;
	// 플레이어의 현재 y 좌표를 반환합니다.
    float GetY() const;
    float GetHomeMinX() const;
    float GetHomeMinY() const;
    float GetHomeMaxX() const;
    float GetHomeMaxY() const;

    bool IsInHomeZone(float targetX, float targetY);
};