#pragma once

class Bullet {
private:
    bool active = false;
    float x = 0;
    float y = 0;
    float dirX = 0;
    float dirY = 0;
    float speed = 700.0f;
public:
	// 총알을 발사합니다. owner는 발사한 플레이어의 인덱스, startX와 startY는 발사 위치, dx와 dy는 방향 벡터입니다.
    void Fire(float startX, float startY, float dx, float dy);
	// 총알의 위치를 업데이트합니다. dt는 경과 시간(초)입니다.
    void Update(float dt);
	// 총알을 비활성화합니다.
    void Clear();
	// 총알이 활성 상태인지 확인합니다.
    bool IsActive() const;
	// 총알의 현재 x 좌표를 반환합니다.
    float GetX() const;
	// 총알의 현재 y 좌표를 반환합니다.
    float GetY() const;
};