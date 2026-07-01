#pragma once
#include <vector>

class SpatialGrid {
public:
    void Init(float worldW, float worldH, float cell);

    void Add(int id, float x, float y);
    void Remove(int id, float x, float y);           // 이전 좌표로 셀 찾아 제거
    void Move(int id, float ox, float oy, float nx, float ny); // 셀 바뀔 때만 이동

    // 3x3 조회: (x,y) 주변 후보 id들을 out에 채움
    void QueryNeighbors(float x, float y, std::vector<int>& out) const;

private:
    int CellIndex(float x, float y) const;
    int cols = 0, rows = 0;
    float cell = 0;
    std::vector<std::vector<int>> buckets;
};