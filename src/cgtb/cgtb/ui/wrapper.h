#pragma once

#include <string>
#include <string_view>
#include <functional>

#include <cgtb/ui.h>
#include <cgtb/ui/canvas.h>

struct NVGcontext;

namespace cgtb::ui {

	struct element;

	class wrapper {

		protected:

		std::string uuid;

		const std::function<canvas::action(const element &element)> poll;
		const std::function<void(NVGcontext *nvgc, const element &element)> render;

		virtual canvas::action poll_cb(const element &element) = 0;
		virtual void render_cb(NVGcontext *nvgc, const element &element) = 0;

		public:

		area body;

		wrapper(const std::string_view &uuid);

		int emit(canvas *canvas);
	};
}