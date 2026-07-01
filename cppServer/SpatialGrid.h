#pragma once
#include <vector>

class SpatialGrid {
public:
    void Init(float worldW, float worldH, float cell);
    void Clear();
    void Add(int id, float x, float y);
    void QueryNeighbors(float x, float y, std::vector<int>& out) const;

private:
    int CellIndex(float x, float y) const;
    int cols = 0, rows = 0;
    float cell = 0;
    std::vector<std::vector<int>> buckets;
};