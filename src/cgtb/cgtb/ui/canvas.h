#pragma once

struct NVGcontext;

#include <vector>
#include <string>
#include <string_view>
#include <functional>
#include <map>

#include <cgtb/ui.h>

namespace cgtb::ui {

	struct element;

	struct canvas {

		// If true, the render target will be cleared to black on the next end().
		// Used when the OS might've destroyed contents of the buffer.
		// Usually not necessary for off-screen render targets.
		// Probably also want to add the whole screen to 'dirty'.
		bool clear = false;

		// The NanoVG context to use for rendering.
		// This is passed to all elements when calling their render().
		NVGcontext *nvgc = 0;

		// Usually the size of the render texture or buffer that will be rendered to.
		// Though, it can be any arbitrary size.
		// These values are passed to nvgBegin() within begin().
		point size;

		// The location of the mouse cursor relative to 'size'.
		point cursor;

		// If false, 'cursor' will be ignored.
		// As in, it doesn't exist.
		// Used when the mouse has travelled off-screen
		bool cursor_enabled = false;

		// 'proposed' are the elements generated this frame via emit().
		// 'finalized' are the elements generated last frame.
		// The vector is to sort them by layer and the map by UUID.
		std::vector<std::map<std::string, element>> proposed, finalized;

		// Used to keep track of what areas of the render target need to be re-drawn.
		// Each pair holds an integer marking the intended layer.
		// All layers above the intended layer are also considered dirty.
		std::vector<std::pair<int, area>> dirty;

		// Prepare canvas for a new series of emit()'s.
		void begin();

		// Updates all element states and calls their poll()'s.
		// Determines which areas are dirty.
		// Initiates element render()'s as needed.
		// Returns true if anything was re-drawn.
		bool end();

		enum action {

			// Do not mark the area as dirty. Do not redraw.
			nothing,

			// Mark the area as dirty so it gets redrawn. Upper layers will also be considered dirty.
			redraw, 

			// Same as 'redraw', but consider lower layers to be dirty as well.
			// Use this if you're utilizing the alpha channel.
			redraw_deep
		};

		// The UUID is to determine if any previously emit()'d element and this one are actually the same.
		// The 'body' is a box representing the area of the screen the element exists in.
		// See 'element::poll' and 'element::render' for info on what those do.
		// If the element is determined to have "moved" then the respective areas of the screen are marked as dirty.
		// The element will be added to the 'proposed' element's and this function will return which layer it exists on.
		// Don't use the same UUID for two distinct elements.
		int emit(const std::string_view &uuid, const area &body, std::function<action(const element &)> poll, std::function<void(NVGcontext *, const element &)> render);
	};
}