#pragma once
#include <vector>
#include <queue>
#include <random> 
#include <cmath>
#include <limits>

struct Vec2 { float x = 0, y = 0; };

class Map {
public:
    void Generate(int widthCells, int heightCells,
        int wallCount, int playerCount, unsigned int seed);

    bool IsWall(float worldX, float worldY) const;

    bool DamageWall(float worldX, float worldY);

    int  WorldToCellX(float worldX) const;
    int  WorldToCellY(float worldY) const;

    int   GetWidthCells()  const { return width; }
    int   GetHeightCells() const { return height; }
    float GetWorldWidth()  const { return width * CELL_SIZE; }
    float GetWorldHeight() const { return height * CELL_SIZE; }

    const std::vector<int>& GetCells() const { return cells; }
    const std::vector<Vec2>& GetSpawnPoints() const { return spawnPoints; }

    static constexpr float CELL_SIZE = 50.0f; 

    bool HasLineOfSight(float x0, float y0, float x1, float y1) const;

    // 선분 (x0,y0)->(x1,y1) 이동 중 처음 부딪히는 벽 셀을 찾는다.
    // 있으면 그 셀 좌표를 hitCx/hitCy 에 담아 true. (경로 전체 검사 + 대각 코너 밀봉)
    bool SegmentHitsWall(float x0, float y0, float x1, float y1,
        int& hitCx, int& hitCy) const;

private:
    int width = 0; 
    int height = 0; 

    std::vector<int> cells;

    std::vector<Vec2> spawnPoints;

    void PlaceRandomWalls(int wallCount, std::mt19937& rng);
    void GenerateSpawnPoints(int playerCount, std::mt19937& rng);
    bool IsFullyConnected() const;

    bool InBounds(int cx, int cy) const;
    bool IsWallCell(int cx, int cy) const;
    int  CellIndex(int cx, int cy) const { return cy * width + cx; }
};