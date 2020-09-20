#pragma once

struct NVGcontext;

#include <chrono>
#include <vector>
#include <memory>
#include <string>
#include <string_view>
#include <functional>
#include <array>
#include <map>
#include <any>

#include <glm/vec2.hpp>

namespace sandfox::ui {

	enum render_action { nothing, redraw, redraw_deep };

	struct element {

		element *prev = 0;
		std::any data;
		glm::vec2 ul, lr;
		glm::vec2 cur;
		bool hover = false;
		std::array<bool, 8> click { };
		std::array<bool, 8> press { };
		std::function<render_action(element *)> poll;
		std::function<void(NVGcontext *, element *)> render;
	};

	struct reactor {

		std::chrono::high_resolution_clock::time_point last;
		std::array<std::vector<std::pair<glm::vec2, double>>, 8> click;
		std::array<double, 8> click_attack, click_decay;
		glm::vec2 contact;
		double hover = 0, hover_attack = 1, hover_decay = 1;
		std::array<double, 8> punch;
		std::array<double, 8> punch_attack, punch_decay;
		std::array<double, 8> press;
		std::array<double, 8> press_attack, press_decay;

		reactor();

		void update(const element &element);
	};

	struct canvas {

		struct shared_state {

			NVGcontext *nvgc = 0;
			glm::vec2 size, cur;
			bool cursor_enabled = false;
			std::array<bool, 8> mouse_buttons { };
			std::array<bool, 8> mouse_buttons_previous { };
		};

		std::shared_ptr<shared_state> state;
		std::vector<std::map<std::string, element>> proposed, finalized;
		std::map<std::string, std::pair<int, element>> absent;
		std::vector<std::pair<int, std::pair<glm::vec2, glm::vec2>>> dirty;

		void begin();
		bool end();

		int emit(const std::string_view &uuid, const glm::vec2 &ul, const glm::vec2 &lr, std::function<render_action(element *)> poll, std::function<void(NVGcontext *, element *)> render, std::any data = { });
	};
}