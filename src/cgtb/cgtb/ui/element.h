#pragma once

#include <memory>
#include <functional>

#include <cgtb/ui.h>
#include <cgtb/ui/canvas.h>

struct NVGcontext;

namespace cgtb::ui {

	class wrapper;

	struct element {

		// The contents of this element as it existed the last time canvas::end() was called.
		element *previous = 0;

		std::weak_ptr<wrapper> wrapped;

		// The area of the screen that this element encompasses.
		area body;

		// The current position of the mouse cursor relative to this element.
		point cursor;
	
		// How many seconds the mouse has been hovering over this element.
		float hover = 0;

		// false - 'hover' will reset to 0 instantly when the cursor leaves.
		// true - 'hover' will decrease by 'hover_decay' per second (until 0) when the cursor leaves.
		// Used for animations or effects.
		float hover_decay_enabled = false;

		// See 'hover_decay_enabled' for explanation.
		float hover_decay = 1;

		// How many seconds the primary mouse button has been pressed down on this element.
		float press = 0;

		// This works the same way as 'hover_decay_enabled'
		// See 'hover_decay_enabled' for explanation.
		float press_decay_enabled = false;

		// See 'hover_decay_enabled' for explanation.
		float press_decay = 1;

		// Called during canvas::end(), every time.
		// Used to give element's a chance to check states and decide if they want to be re-drawn.
		// The element will return true if it wants to be re-drawn.
		std::function<canvas::action(const element &)> poll;

		// This element is within a section of the screen that's been considered 'dirty'.
		// It will use this function to re-draw itself.
		std::function<void(NVGcontext *, const element &)> render;
	};
}