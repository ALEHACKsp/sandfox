#pragma once

#include <array>
#include <vector>
#include <chrono>

#include <cgtb/ui.h>

namespace cgtb::ui {

	struct element;

	struct reactor {

		std::chrono::system_clock::time_point last;

		std::array<std::vector<std::pair<cgtb::ui::point, double>>, 8> click;
		std::array<double, 8> click_attack, click_decay;

		cgtb::ui::point contact;

		double hover = 0, hover_attack = 1, hover_decay = 1;

		std::array<double, 8> punch;
		std::array<double, 8> punch_attack, punch_decay;

		std::array<double, 8> press;
		std::array<double, 8> press_attack, press_decay;

		reactor();

		void update(const cgtb::ui::element &element);
	};
}