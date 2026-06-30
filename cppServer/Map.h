#pragma once
#include <vector>
#include <queue>
#include <random> 

struct Vec2 { float x = 0, y = 0; };

class Map {
public:
    // ── 생성 ──
    // 맵을 생성한다. 랜덤 벽 배치 + 연결성 보장 + 스폰 좌표까지 한 번에.
    void Generate(int widthCells, int heightCells,
        int wallCount, int playerCount, unsigned int seed);

    // ── 질의 (월드 좌표 기준, 충돌용) ──
    // 해당 월드 좌표가 벽인지. 맵 밖도 벽으로 취급(= 못 나감).
    bool IsWall(float worldX, float worldY) const;

    // ── 벽 파괴 ──
    // 월드 좌표의 벽에 데미지. 이번에 파괴됐으면 true 반환(→ 파괴 패킷 전송용).
    bool DamageWall(float worldX, float worldY);

    // ── 좌표 변환 ──
    int  WorldToCellX(float worldX) const;
    int  WorldToCellY(float worldY) const;

    // ── 접근자 ──
    int   GetWidthCells()  const { return width; }
    int   GetHeightCells() const { return height; }
    float GetWorldWidth()  const { return width * CELL_SIZE; }
    float GetWorldHeight() const { return height * CELL_SIZE; }

    // 클라 전송용: 벽 격자 원본 (한 번 보낼 때)
    const std::vector<int>& GetCells() const { return cells; }
    // 스폰 좌표 (게임 시작 시 플레이어 배치용)
    const std::vector<Vec2>& GetSpawnPoints() const { return spawnPoints; }

    static constexpr float CELL_SIZE = 50.0f;   // 칸 하나 = 월드 50유닛

private:
    int width = 0;     // 가로 칸 수
    int height = 0;     // 세로 칸 수

    std::vector<int> cells;

    std::vector<Vec2> spawnPoints;

    // ── 생성 내부 단계 ──
    void PlaceRandomWalls(int wallCount, std::mt19937& rng);
    void GenerateSpawnPoints(int playerCount, std::mt19937& rng);
    bool IsFullyConnected() const;

    // ── 셀 단위 헬퍼 ──
    bool InBounds(int cx, int cy) const;
    bool IsWallCell(int cx, int cy) const;
    int  CellIndex(int cx, int cy) const { return cy * width + cx; }
};