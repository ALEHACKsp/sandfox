#pragma once

#include <functional>
#include <array>

#include <cgtb/ui.h>
#include <cgtb/ui/canvas.h>

struct NVGcontext;

namespace cgtb::ui {

	struct element {

		// The contents of this element as it existed the last time canvas::end() was called.
		element *previous = 0;

		// Optional user-specified ptr that can be reference during the callbacks.
		void *state = 0;

		// The area of the screen that this element encompasses.
		area body;

		// The current position of the mouse cursor relative to this element.
		point cursor;

		bool hover = false;

		std::array<bool, 8> click { };
		std::array<bool, 8> press { };

		// Called during canvas::end(), every time.
		// Used to give element's a chance to check states and decide if they want to be re-drawn.
		// The element will return true if it wants to be re-drawn.
		std::function<canvas::action(const element &, void *)> poll;

		// This element is within a section of the screen that's been considered 'dirty'.
		// It will use this function to re-draw itself.
		std::function<void(NVGcontext *, const element &, void *)> render;
	};
}