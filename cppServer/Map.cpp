#include"Map.h"

void Map::Generate(int widthCells, int heightCells,
    int wallCount, int playerCount, unsigned int seed) {
    width = widthCells;
    height = heightCells;

    int attempt = 0;
    while (true) {
        cells.assign(width * height, 0);

        std::mt19937 rng(seed + attempt);
        PlaceRandomWalls(wallCount, rng);

        if (IsFullyConnected()) break;
        attempt++;
    }
    std::mt19937 spawnRng(seed);
    GenerateSpawnPoints(playerCount, spawnRng);
}

void Map::PlaceRandomWalls(int wallCount, std::mt19937& rng) {
    std::uniform_int_distribution<int> distX(0, width - 1);
    std::uniform_int_distribution<int> distY(0, height - 1);

    int placed = 0;

    while (placed < wallCount) {
        int cx = distX(rng);
        int cy = distY(rng);
        int idx = CellIndex(cx, cy);

        if (cells[idx] != 0) continue;

        cells[idx] = 1;
        placed++;
    }
}

bool Map::IsFullyConnected() const {
    int totalEmpty = 0;
    int startIdx = -1;

    for (int i = 0; i < width * height; i++) {
        if (cells[i] == 0) {
            totalEmpty++;
            if (startIdx == -1) startIdx = i;
        }
    }

    if (startIdx == -1) return false;

    std::vector<bool> visited(width * height, false);
    std::queue<int> q;

    q.push(startIdx);
    visited[startIdx] = true;
    int visitedCount = 0;

    int dx[] = { 0, 0, -1, 1 };
    int dy[] = { -1, 1, 0, 0 };

    while (!q.empty()) {
        int curr = q.front();
        q.pop();
        visitedCount++;

        int cx = curr % width;
        int cy = curr / width;

        for (int i = 0; i < 4; i++) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];

            if (!InBounds(nx, ny)) continue;

            int nextIdx = CellIndex(nx, ny);

            if (visited[nextIdx] || cells[nextIdx] != 0) continue;

            visited[nextIdx] = true;
            q.push(nextIdx);
        }
    }

    return visitedCount == totalEmpty;
}

void Map::GenerateSpawnPoints(int playerCount, std::mt19937& rng) {
    std::uniform_int_distribution<int> distX(0, width - 1);
    std::uniform_int_distribution<int> distY(0, height - 1);

    spawnPoints.clear();

    while ((int)spawnPoints.size() < playerCount) {
        int cx = distX(rng);
        int cy = distY(rng);
        if (cells[CellIndex(cx, cy)] != 0) continue;

        Vec2 p;
        p.x = cx * CELL_SIZE + CELL_SIZE / 2.0f;
        p.y = cy * CELL_SIZE + CELL_SIZE / 2.0f;

        spawnPoints.push_back(p);
    }
}

bool Map::IsWall(float worldX, float worldY) const {
    return IsWallCell(WorldToCellX(worldX), WorldToCellY(worldY));
}

bool Map::DamageWall(float worldX, float worldY) {
    int cx = WorldToCellX(worldX);
    int cy = WorldToCellY(worldY);
    if (!InBounds(cx, cy)) return false;
    int idx = CellIndex(cx, cy);
    if (cells[idx] == 0) return false;
    cells[idx]--;
    return cells[idx] == 0;
}

int  Map::WorldToCellX(float worldX) const { 
    return static_cast<int>(std::floor(worldX / CELL_SIZE));
}
int  Map::WorldToCellY(float worldY) const { 
    return static_cast<int>(std::floor(worldY / CELL_SIZE));
}
bool Map::InBounds(int cx, int cy) const { return cx >= 0 && cx < width && cy >= 0 && cy < height; }
bool Map::IsWallCell(int cx, int cy) const {
    if (!InBounds(cx, cy)) return true; 
    return cells[CellIndex(cx, cy)] != 0;
}

bool Map::HasLineOfSight(float x0, float y0, float x1, float y1) const
{
    int cx = WorldToCellX(x0);
    int cy = WorldToCellY(y0);

    int tx = WorldToCellX(x1);
    int ty = WorldToCellY(y1);

    // 같은 셀이라면 바로 보임
    if (cx == tx && cy == ty)
        return true;

    float dx = x1 - x0;
    float dy = y1 - y0;

    int stepX = (dx > 0) - (dx < 0);
    int stepY = (dy > 0) - (dy < 0);

    const float INF = std::numeric_limits<float>::infinity();

    float tDeltaX = (stepX == 0) ? INF : CELL_SIZE / std::abs(dx);
    float tDeltaY = (stepY == 0) ? INF : CELL_SIZE / std::abs(dy);

    float tMaxX;
    if (stepX > 0)
        tMaxX = ((cx + 1) * CELL_SIZE - x0) / std::abs(dx);
    else if (stepX < 0)
        tMaxX = (x0 - cx * CELL_SIZE) / std::abs(dx);
    else
        tMaxX = INF;

    float tMaxY;
    if (stepY > 0)
        tMaxY = ((cy + 1) * CELL_SIZE - y0) / std::abs(dy);
    else if (stepY < 0)
        tMaxY = (y0 - cy * CELL_SIZE) / std::abs(dy);
    else
        tMaxY = INF;

    while (true)
    {
        if (tMaxX < tMaxY)
        {
            cx += stepX;
            tMaxX += tDeltaX;
        }
        else
        {
            cy += stepY;
            tMaxY += tDeltaY;
        }

        // 목적지 셀에 도착
        if (cx == tx && cy == ty)
            return true;

        // 중간 셀이 벽이면 시야 차단
        if (IsWallCell(cx, cy))
            return false;
    }
}