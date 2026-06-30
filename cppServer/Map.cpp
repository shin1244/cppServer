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
}

void Map::PlaceRandomWalls(int wallCount, std::mt19937& rng) {
    std::uniform_int_distribution<int> distX(0, width - 1);
    std::uniform_int_distribution<int> distY(0, height - 1);

    int placed = 0;

    while (placed < wallCount) {
        int cx = distX(rng);
        int cy = distY(rng);
        int idx = cy * width + cx;

        if (cells[idx] != 0) continue;

        cells[idx] = 10;
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
            if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
            int nextIdx = ny * width + nx;

            if (nextIdx >= 0 && nextIdx < width * height 
                && !visited[nextIdx] && cells[nextIdx] == 0) {
                visited[nextIdx] = true;
                q.push(nextIdx);
            }
        }
    }
    return visitedCount == totalEmpty;
}