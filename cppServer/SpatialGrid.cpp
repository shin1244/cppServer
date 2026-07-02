#include"SpatialGrid.h"

void SpatialGrid::Init(float worldW, float worldH, float cellSize) {
	cell = cellSize;
	cols = (int)ceil(worldW / cell);
	rows = (int)ceil(worldH / cell);
	buckets.resize(cols * rows);
}

void SpatialGrid::Clear() {
	for (int i = 0; i < cols * rows; i++)
		buckets[i].clear();
}

void SpatialGrid::Add(int id, float x, float y) {
	int idx = CellIndex(x, y);
	buckets[idx].push_back(id);
}

void SpatialGrid::QueryNeighbors(float x, float y, std::vector<int>& out) const {
	int cx = int(x / cell);
	int cy = int(y / cell);

	if (cx < 0) cx = 0; else if (cx >= cols) cx = cols - 1;
	if (cy < 0) cy = 0; else if (cy >= rows) cy = rows - 1;

	for (int dy = -1; dy <= 1; ++dy) {
		int ny = cy + dy;
		if (ny < 0 || ny >= rows) continue;
		for (int dx = -1; dx <= 1; ++dx) {
			int nx = cx + dx;
			if (nx < 0 || nx >= cols) continue;
			const auto& bucket = buckets[ny * cols + nx];
			out.insert(out.end(), bucket.begin(), bucket.end());
		}
	}
}

int SpatialGrid::CellIndex(float x, float y) const {
	int cx = int(x / cell);
	int cy = int(y / cell);

	if (cx < 0) cx = 0; else if (cx >= cols) cx = cols - 1;
	if (cy < 0) cy = 0; else if (cy >= rows) cy = rows - 1;

	return cy * cols + cx;
}
