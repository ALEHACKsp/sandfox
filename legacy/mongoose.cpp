#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES2/gl2.h>
#else
#include <glad.h>
#endif

#include <GLFW/glfw3.h>

#include "nanovg.h"
#define NANOVG_GLES2_IMPLEMENTATION
#include "nanovg_gl.h"

#include <memory>
#include <functional>

using namespace std::placeholders;

#include <spdlog/spdlog.h>

GLFWwindow *glfw_window = 0;
NVGcontext *nvg_context = 0;

#include <sandfox/ui/element.h>
#include <sandfox/ui/canvas.h>
#include <sandfox/ui/reactor.h>

sandfox::ui::canvas canvas;

#include <sandfox/sig.h>

GLFWcursor *cur_active = 0, *cur_wanted = 0, *cur_normal = 0, *cur_select = 0;

void update_cursor(GLFWwindow *window) {
	if (cur_wanted != cur_active) {
		glfwSetCursor(window, cur_wanted);
		cur_active = cur_wanted;
	}
	cur_wanted = 0;
}

bool render() {

	canvas.begin();

	auto render_man = std::make_shared<int>();
	auto ui_element = std::make_shared<int>();

	spdlog::info("render_man:{} ui_element:{}", reinterpret_cast<void *>(render_man.get()), reinterpret_cast<void *>(ui_element.get()));

	syn::connect<sandfox::ui::poll>(ui_element, render_man, [](int *compositor) {
		spdlog::info("Synapse: {}", reinterpret_cast<void *>(compositor));
	});

	// The compositor could emit using the pointer to the UI element.
	syn::emit<sandfox::ui::poll>(ui_element.get());

	exit(12);

	canvas.emit(
		
		"bg_tile",

		{
			0,
			0,
			canvas.state->size.x,
			canvas.state->size.y
		},
		
		[](const sandfox::ui::element &element, void *state) {
			return sandfox::ui::canvas::nothing;
		},

		[](NVGcontext *nvgc, const sandfox::ui::element &element, void *state) {
			nvgBeginPath(nvgc);
			nvgRect(
				nvgc,
				element.body.x1, element.body.y1,
				element.body.x2 - element.body.x1,
				element.body.y2 - element.body.y1
			);
			nvgFillColor(nvgc, nvgRGB(64, 32, 16));
			nvgFill(nvgc);
		}
	);

	/*

	struct floating_button : sandfox::ui::reactor {

		const std::string uuid; //, label;

		sandfox::ui::area body;
		// std::string icon_name;

		floating_button(const std::string_view &uuid, const std::string_view &label) : uuid(uuid) { //, label(label) {
			hover_attack = 4;
			hover_decay = 4;
			click_decay[0] = 3;
		}

		sandfox::ui::canvas::action poll(const sandfox::ui::element &element, void *state) {
			auto self = reinterpret_cast<floating_button *>(state);
			self->update(element);
			if (element.hover) cur_wanted = cur_select;
			if (self->click[0].size() || (self->hover != 0 && self->hover != 1) || (element.hover && element.previous && element.cursor != element.previous->cursor)) return sandfox::ui::canvas::redraw_deep;
			return sandfox::ui::canvas::nothing;
		}

		void render(NVGcontext *nvgc, const sandfox::ui::element &element, void *state) {

			auto self = reinterpret_cast<floating_button *>(state);

			nvgBeginPath(nvgc);
			nvgRoundedRect(
				nvgc,
				element.body.x1, element.body.y1,
				element.body.x2 - element.body.x1,
				element.body.y2 - element.body.y1,
				3
			);

			float brightness = 160 - (48 * self->hover);

			nvgFillColor(nvgc, nvgRGBA(brightness, brightness, brightness, 128));
			nvgFill(nvgc);

			nvgBeginPath(nvgc);
			nvgRoundedRect(
				nvgc,
				element.body.x1, element.body.y1,
				element.body.x2 - element.body.x1,
				element.body.y2 - element.body.y1,
				3
			);

			for (auto &click : self->click[0]) {

				float power = (click.second * click.second) / (2.0f * ((click.second * click.second) - click.second) + 1.0f);
				float b = std::max(element.body.x2 - element.body.x1, element.body.y2 - element.body.y1);

				auto click_paint = nvgRadialGradient(
					nvgc,
					element.body.x1 + click.first.x,
					element.body.y1 + click.first.y,
					b * (1.0f - click.second),
					b * (1.0f - click.second),
					nvgRGBA(255, 255, 255, 96.0f * power),
					nvgRGBA(0, 0, 0, 0)
				);

				nvgFillPaint(nvgc, click_paint);
				nvgFill(nvgc);
			}

			auto icon = get_icon(nvgc, self->icon_name, 24, 24);
			auto paint = nvgImagePattern(nvgc, element.body.x1 + 5, element.body.y1 + 5, 24, 24, 0, icon, 1);

			paint.innerColor = nvgRGBA(8, 8, 8, 255);

			nvgBeginPath(nvgc);
			nvgRect(nvgc, element.body.x1 + 5, element.body.y1 + 5, 24, 24);
			nvgFillPaint(nvgc, paint);
			nvgFill(nvgc);

			nvgFillColor(nvgc, nvgRGB(192, 192, 192));
			nvgFontFaceId(nvgc, get_font(nvgc, "roboto"));
			nvgFontSize(nvgc, 16);
			nvgTextAlign(nvgc, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgText(nvgc, element.body.x1 + ((element.body.x2 - element.body.x1) * 0.5), element.body.y1 + ((element.body.y2 - element.body.y1) * 0.5), self->label.data(), 0);

		}

		void emit(sandfox::ui::canvas &canvas) {

			canvas.emit(
				uuid,
				{ body.x1, body.y1, body.x2, body.y2 },
				std::bind(&floating_button::poll, this, _1, _2),
				std::bind(&floating_button::render, this, _1, _2, _3),
				this
			);
		}
	};

	static floating_button buttons[] {

		{ "the_button", "Start" },
		{ "the_second_button", "Continue" },
		{ "the_last_but_not_least_button", "Quit" }
	};

	for (int i = 0; i < 3; i++) {

		switch (i) {
			case 0: buttons[i].icon_name = "alien"; break;
			case 1: buttons[i].icon_name = "atom"; break;
			case 2: buttons[i].icon_name = "bolt"; break;
		}

		buttons[i].body = { 10, 10 + (38 * i), 210, 10 + 34 + (38 * i) };
	}

	buttons[0].emit(canvas);
	buttons[1].emit(canvas);

	if (canvas.dirty.size()) buttons[2].emit(canvas);

	*/

	if (canvas.end()) {
		glFinish();
		spdlog::debug("Update");
		return true;
	} else return false;
}

void window_refresh_callback(GLFWwindow *window) {
	spdlog::debug("Refresh request");
	glClear(GL_COLOR_BUFFER_BIT);
	canvas.dirty.push_back({ 0, { 0, 0, canvas.state->size.x, canvas.state->size.y } });
	render();
}

void window_size_callback(GLFWwindow *window, int width, int height) {
	spdlog::debug("Framebuffer resize: {} by {}", width, height);
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, width, height);
	canvas.dirty.push_back({ 0, { 0, 0, canvas.state->size.x, canvas.state->size.y } });
	canvas.state->size = { width, height };
}

void cursor_location_callback(GLFWwindow *window, double x, double y) {
	canvas.state->cursor = { static_cast<int>(x), static_cast<int>(y) };
}

void cursor_enter_callback(GLFWwindow *window, int entered) {
	canvas.state->cursor_enabled = entered;
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
	canvas.state->mouse_buttons[button] = action;
}

void main_loop() {

	int current_framebuffer_width, current_framebuffer_height;
	glfwGetFramebufferSize(glfw_window, &current_framebuffer_width, &current_framebuffer_height);

	#ifdef __EMSCRIPTEN__
	{
		double current_css_width, current_css_height;
		emscripten_get_element_css_size("canvas", &current_css_width, &current_css_height);
		if (static_cast<int>(current_css_width) != current_framebuffer_width || static_cast<int>(current_css_height) != current_framebuffer_height) {
			glfwSetWindowSize(glfw_window, current_css_width, current_css_height);
			current_framebuffer_width = current_css_width;
			current_framebuffer_height = current_css_height;
			spdlog::debug("CSS element resize: {} by {}", current_css_width, current_css_height);
		}
	}
	#endif

	canvas.state->size = { current_framebuffer_width, current_framebuffer_height };

	glfwWaitEventsTimeout(render() ? 0.02 : 0.008);

	update_cursor(glfw_window);
}

int main() {

	spdlog::set_level(spdlog::level::debug);

	glfwSetErrorCallback([](int code, const char *what) {
		spdlog::error("GLFW error #{}: {}", code, what);
	});

	spdlog::debug("Init GLFW v{}.{}.{}", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION);

	if (!glfwInit()) {
		spdlog::error("GLFW init failure");
		return 102;
	}

	#ifndef __EMSCRIPTEN__
	std::atexit([]() {
		spdlog::debug("Shutdown GLFW");
		glfwTerminate();
	});
	#endif

	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);

	glfw_window = glfwCreateWindow(800, 600, "GLFW3", 0, 0);

	if (!glfw_window) {
		spdlog::error("GLFW window creation failure");
		return 103;
	}

	spdlog::debug("GLFW window @ {}", reinterpret_cast<void *>(glfw_window));

	cur_normal = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	cur_select = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

	glfwSetWindowRefreshCallback(glfw_window, window_refresh_callback);
	glfwSetFramebufferSizeCallback(glfw_window, window_size_callback);
	glfwSetCursorPosCallback(glfw_window, cursor_location_callback);
	glfwSetCursorEnterCallback(glfw_window, cursor_enter_callback);
	glfwSetMouseButtonCallback(glfw_window, mouse_button_callback);
	glfwMakeContextCurrent(glfw_window);

	#ifndef __EMSCRIPTEN__
	if (!gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		spdlog::error("GLES2 load failure");
		return 104;
	}
	#endif

	spdlog::debug("GL version: {}", glGetString(GL_VERSION));
	spdlog::debug("GL renderer: {}", glGetString(GL_RENDERER));

	nvg_context = nvgCreateGLES2(NVG_ANTIALIAS);

	if (!nvg_context) {
		spdlog::error("NVG context creation failure");
		return 105;
	}

	spdlog::debug("NVG context @ {}", reinterpret_cast<void *>(nvg_context));

	#ifndef __EMSCRIPTEN__
	std::atexit([]() {
		spdlog::debug("Destroy NVG context @ {}", reinterpret_cast<void *>(nvg_context));
		nvgDeleteGLES2(nvg_context);
		nvg_context = 0;
	});
	#endif

	canvas.state = std::make_shared<sandfox::ui::canvas::shared_state>();
	canvas.state->nvgc = nvg_context;

	#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(main_loop, 0, true);
	#else
	while (!glfwWindowShouldClose(glfw_window)) main_loop();
	#endif

	spdlog::info("Init okay");

	return 0;
}