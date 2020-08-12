#pragma once

namespace cgtb::ui {

	// Stores an X and a Y value.
	struct point {
		int x = 0, y = 0;
		bool operator==(const point &other) const;
		bool operator!=(const point &other) const;
	};

	// Stores x1, y1, x2, y2.
	struct area {
		int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
		bool intersecting(const area &other) const;
		bool contains(const point &ref);
		bool operator==(const area &other) const;
		bool operator!=(const area &other) const;
	};
}