#include "Item.h"

void Item::Spawn(float sx, float sy, int t) {
	active = true;
	x = sx;
	y = sy;
	type = t;
}
void Item::Clear() { active = false; }
bool Item::IsActive() const { return active; }
float Item::GetX() const { return x; }
float Item::GetY() const { return y; }
int Item::GetType() const { return type; }
bool Item::ContainsPoint(float px, float py, float radius) const {
	if (!active) return false;
	float dx = x - px, dy = y - py;
	return dx * dx + dy * dy <= radius * radius;
}