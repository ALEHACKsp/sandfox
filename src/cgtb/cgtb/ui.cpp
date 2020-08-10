#include "ui.h"

bool cgtb::ui::point::operator==(const point &other) const {
	return x == other.x && y == other.y;
}

bool cgtb::ui::point::operator!=(const point &other) const {
	return !(*this==other);
}

bool cgtb::ui::area::intersecting(const area &other) const {
	return x1 < other.x2 && x2 > other.x1 && y1 < other.y2 && y2 > other.y1;
}

bool cgtb::ui::area::contains(const point &ref) {
	return ref.x >= x1 && ref.x < x2 && ref.y >= y1 && ref.y < y2;
}

bool cgtb::ui::area::operator==(const area &other) const {
	return x1 == other.x1 && y1 == other.y1 && x2 == other.x2 && y2 == other.y2;
}

bool cgtb::ui::area::operator!=(const area &other) const {
	return !(*this==other);
}