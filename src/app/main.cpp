#include <iostream>
#include <sstream>
#include <thread>
#include <array>
#include <map>

#include <cgtb/third-party/glad.h>
#define NANOVG_GL2_IMPLEMENTATION
#include <cgtb/third-party/nanovg.h>
#include <cgtb/third-party/nanovg_gl.h>
#include <cgtb/third-party/stb_image.h>
#include <cgtb/third-party/glfw/include/GLFW/glfw3.h>

#if defined(_WIN32) || defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <cgtb/ui/canvas.h>
#include <cgtb/ui/element.h>
#include <cgtb/icons.h>

#include <fmt/core.h>

#include <boost/synapse/connect.hpp>
#include <boost/synapse/connection.hpp>

#include "glfw_synapse.h"

//

using fmt::format;
using std::cout;
using std::endl;

using namespace std::placeholders;

// Since we're not rendering to a texture, but right to the backbuffer
// we have to handle double-buffering.

// Due to double-buffering, two separate UI canvas's need to be managed.
// This ensures that both buffers are individually updated as needed.

// In the case of triple-buffering, three separate UI canvas's would be needed.

int swap_buffer_index = 0;
cgtb::ui::canvas swap_buffer_canvas[2];
cgtb::ui::canvas *current_canvas = &swap_buffer_canvas[0];

GLFWcursor *cur_active = 0, *cur_wanted = 0, *cur_normal = 0, *cur_select = 0;

std::map<std::string, int> gl_icon_textures;

int make_icon(NVGcontext *context, const std::string_view &name) {
	if (!cur_normal) cur_normal = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	if (!cur_select) cur_select = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
	auto existing_texture_it = gl_icon_textures.find(name.data());
	if (existing_texture_it != gl_icon_textures.end()) return existing_texture_it->second;
	auto image_data_it = cgtb::icons.find(name.data());
	if (image_data_it == cgtb::icons.end()) return 0;
	int new_image = nvgCreateImageMem(context, 0, image_data_it->second.first, image_data_it->second.second);
	if (!new_image) return 0;
	gl_icon_textures[name.data()] = new_image;
	cout << "New icon '" << name << "'; GL texture #" << new_image << endl;
	return new_image;
}

void show_error_message(const std::string_view &what) {
	#if defined(WIN32_LEAN_AND_MEAN) && !defined(__EMSCRIPTEN__)
	MessageBoxA(NULL, what.data(), "Error", MB_ICONERROR);
	#else
	std::cerr << what.data() << endl;
	#endif
}

void exit_with_error_if(bool state, const char *what, int code) {
	if (!state) return;
	std::stringstream ss;
	ss << what << " ";
	ss << "This program is unable to continue due to this error and must exit. ";
	ss << "The identifier for this error is #" << code << ".";
	show_error_message(ss.str());
	exit(code);
}

void glfw_error_callback(int code, const char *what) {
	std::stringstream ss;
	ss << what << ". (GLFW error #" << code << ")";
	show_error_message(ss.str());
}

void render(GLFWwindow *window) {

	// Update the canvas pointer to reflect the canvas that's associated
	// with the current swap buffer.

	current_canvas = &swap_buffer_canvas[swap_buffer_index];
	current_canvas->begin();

	current_canvas->emit(
		
		"bg_tile",

		{
			0,
			0,
			current_canvas->state->size.x,
			current_canvas->state->size.y
		},
		
		[](const cgtb::ui::element &element, void *state) {
			return cgtb::ui::canvas::nothing;
		},

		[](NVGcontext *nvgc, const cgtb::ui::element &element, void *state) {
			nvgBeginPath(nvgc);
			nvgRect(
				nvgc,
				element.body.x1, element.body.y1,
				element.body.x2 - element.body.x1,
				element.body.y2 - element.body.y1
			);
			nvgFillColor(nvgc, nvgRGBA(0, 0, 0, 32));
			nvgFill(nvgc);
		}
	);

	struct advanced_state {

		bool updated_once = false;

		std::chrono::system_clock::time_point last_update { };

		std::array<std::vector<std::pair<cgtb::ui::point, float>>, 8> click;
		std::array<float, 8> click_attack { 1 }, click_decay { 1 };

		cgtb::ui::point contact;

		float hover = 0, hover_attack = 1, hover_decay = 1;

		std::array<float, 8> punch = { };
		std::array<float, 8> punch_attack { 1 }, punch_decay { 1 };

		std::array<float, 8> press = { };
		std::array<float, 8> press_attack { 1 }, press_decay { 1 };

		void update_advanced_state(const cgtb::ui::element &element) {

			float delta = 0.0f;

			auto now = std::chrono::system_clock::now();

			if (updated_once) {
				auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update);
				delta = elapsed.count() * 0.001f;
			} else {
				last_update = now;
				updated_once = true;
			}

			last_update = now;

			if (element.hover) {
				hover = std::min(1.0f, hover + (hover_attack * delta));
				contact = element.cursor;
			} else hover = std::max(0.0f, hover - (hover_decay * delta));

			for (int i = 0; i < 8; i++) {

				for (auto &button_specific_clicks : click) {
					for (int c = 0; c < button_specific_clicks.size(); c++) {
						button_specific_clicks[c].second -= click_decay[i] * delta;
						if (button_specific_clicks[c].second <= 0) {
							button_specific_clicks.erase(button_specific_clicks.begin() + c);
							c--;
						}
					}
				}

				if (element.click[i]) {
					punch[i] = std::min(1.0f, punch[i] + punch_attack[i]);
					click[i].push_back({ element.cursor, click_attack[i] });
				} else punch[i] = std::max(0.0f, punch[i] - (punch_decay[i] * delta));

				if (element.press[i]) press[i] = std::min(1.0f, press[i] + (press_attack[i] * delta));
				else press[i] = std::max(0.0f, press[i] - (press_decay[i] * delta));
			}
		}
	};

	struct floating_button : advanced_state {

		const std::string uuid;

		cgtb::ui::area body;
		std::string icon_name;

		floating_button(const std::string_view &uuid) : uuid(uuid) {
			hover_attack = 24;
			hover_decay = 24;
			punch_decay[0] = 5;
		}

		cgtb::ui::canvas::action poll(const cgtb::ui::element &element, void *state) {
			auto self = reinterpret_cast<floating_button *>(state);
			self->update_advanced_state(element);
			if (element.hover) cur_wanted = cur_select;
			if (self->click[0].size() || (self->hover != 0 && self->hover != 1) || (element.hover && element.previous && element.cursor != element.previous->cursor)) return cgtb::ui::canvas::redraw;
			return cgtb::ui::canvas::nothing;
		}

		void render(NVGcontext *nvgc, const cgtb::ui::element &element, void *state) {
			
			auto self = reinterpret_cast<floating_button *>(state);

			nvgBeginPath(nvgc);
			nvgRect(
				nvgc,
				element.body.x1, element.body.y1,
				element.body.x2 - element.body.x1,
				element.body.y2 - element.body.y1
			);

			float brightness = 160 - (48 * self->hover);

			nvgFillColor(nvgc, nvgRGB(brightness, brightness, brightness));
			nvgFill(nvgc);

			for (auto &click : self->click[0]) {

				float power = (click.second * click.second) / (2.0f * ((click.second * click.second) - click.second) + 1.0f);

				auto click_paint = nvgRadialGradient(
					nvgc,
					element.body.x1 + click.first.x,
					element.body.y1 + click.first.y,
					1200 * (1.0f - click.second),
					1200 * (1.0f - click.second),
					nvgRGBA(255, 255, 255, 96.0f * power),
					nvgRGBA(0, 0, 0, 0)
				);

				nvgFillPaint(nvgc, click_paint);
				nvgFill(nvgc);
			}

			auto icon = make_icon(nvgc, self->icon_name);
			auto paint = nvgImagePattern(nvgc, element.body.x1 + 5, element.body.y1 + 5, 24, 24, 0, icon, 1);

			paint.innerColor = nvgRGB(16, 16, 16);

			nvgBeginPath(nvgc);
			nvgRect(nvgc, element.body.x1 + 5, element.body.y1 + 5, 24, 24);
			nvgFillPaint(nvgc, paint);
			nvgFill(nvgc);
		}

		void emit(cgtb::ui::canvas *canvas) {
			canvas->emit(
				uuid,
				{ body.x1, body.y1, body.x2, body.y2 },
				std::bind(&floating_button::poll, this, _1, _2),
				std::bind(&floating_button::render, this, _1, _2, _3),
				this
			);
		}
	};

	static floating_button buttons[] {
		{ "the_button" },
		{ "the_second_button" },
		{ "the_last_but_not_least_button" }
	};

	for (int i = 0; i < 3; i++) {
		switch (i) {
			case 0: buttons[i].icon_name = "24.material.bug_report"; break;
			case 1: buttons[i].icon_name = "24.material.developer_board"; break;
			case 2: buttons[i].icon_name = "24.material.credit_card"; break;
		}
		buttons[i].body = { 10, 10 + (38 * i), 44, 10 + 34 + (38 * i) };
		buttons[i].emit(current_canvas);
	}

	/*
	for (int side_bar_icon_index = 0; side_bar_icon_index < 8; side_bar_icon_index++) {

		const int padding = 20;
		const int icon_size = 48;

		current_canvas->emit(

			format("side_icon_{}", side_bar_icon_index),

			{
				current_canvas->state->size.x - (padding + icon_size),
				(current_canvas->state->size.y - (padding + icon_size)) - (icon_size * side_bar_icon_index),
				current_canvas->state->size.x - padding,
				(current_canvas->state->size.y - padding) - (icon_size * side_bar_icon_index)
			},

			[](const cgtb::ui::element &element, void *state) {
				if (element.hover) cur_wanted = cur_select;
				if (!element.previous) return cgtb::ui::canvas::nothing;
				if (static_cast<bool>(element.hover) != static_cast<bool>(element.previous->hover)) return cgtb::ui::canvas::redraw_deep;
				return cgtb::ui::canvas::nothing;
			},

			[icon_size](NVGcontext *nvgc, const cgtb::ui::element &element, void *state) {

				nvgBeginPath(nvgc);

				nvgRect(
					nvgc,
					element.body.x1, element.body.y1,
					element.body.x2 - element.body.x1,
					element.body.y2 - element.body.y1
				);

				int icon = make_icon(nvgc, element.hover ? format("{}.material.sentiment_very_satisfied", icon_size) : format("{}.material.sentiment_very_dissatisfied", icon_size));
				auto paint = nvgImagePattern(nvgc, element.body.x1, element.body.y1, icon_size, icon_size, 0, icon, 1);

				paint.innerColor = element.hover ? nvgRGBA(128, 255, 128, 255) : nvgRGBA(255, 128, 128, 128);

				nvgFillPaint(nvgc, paint);
				nvgFill(nvgc);
			}

		);
	}
	*/

	current_canvas->emit(

		"drop_area",

		{
			54,
			10,
			current_canvas->state->size.x - 10,
			120
		},

		[](const cgtb::ui::element &element, void *) {
			return cgtb::ui::canvas::nothing;
		},

		[](NVGcontext *nvgc, const cgtb::ui::element &element, void *) {
		
			nvgBeginPath(nvgc);

			nvgRoundedRect(
				nvgc,
				element.body.x1, element.body.y1,
				element.body.x2 - element.body.x1,
				element.body.y2 - element.body.y1,
				3
			);

			nvgStrokeWidth(nvgc, 2);
			nvgStrokeColor(nvgc, nvgRGBA(0, 0, 0, 128));
			nvgStroke(nvgc);

			auto icon = make_icon(nvgc, "48.material.play_for_work");

			auto paint = nvgImagePattern(
				nvgc,
				element.body.x1 + ((element.body.x2 - element.body.x1) / 2) - 24,
				element.body.y1 + ((element.body.y2 - element.body.y1) / 2) - 24,
				48, 48, 0, icon, 1
			);

			paint.innerColor = nvgRGBA(0, 0, 0, 128);

			nvgBeginPath(nvgc);
			nvgRect(
				nvgc,
				element.body.x1 + ((element.body.x2 - element.body.x1) / 2) - 24,
				element.body.y1 + ((element.body.y2 - element.body.y1) / 2) - 24,
				48, 48
			);
			nvgFillPaint(nvgc, paint);
			nvgFill(nvgc);
		}

	);

	/*
	current_canvas->emit(

		"big_icon",

		{
			10,
			current_canvas->state->size.y - 58,
			58,
			current_canvas->state->size.y - 10
		},

		[](const cgtb::ui::element &element, void *) {
			return cgtb::ui::canvas::nothing;
		},

		[](NVGcontext *nvgc, const cgtb::ui::element &element, void *) {
		
			nvgBeginPath(nvgc);

			nvgRect(
				nvgc,
				element.body.x1, element.body.y1,
				element.body.x2 - element.body.x1,
				element.body.y2 - element.body.y1
			);

			auto paint = nvgImagePattern(nvgc, element.body.x1, element.body.y1, 48, 48, 0, make_icon(nvgc, "48.material.fingerprint"), 1);

			paint.innerColor = nvgRGBA(255, 255, 255, 32);

			nvgFillPaint(nvgc, paint);
			nvgFill(nvgc);
		}

	);
	*/

	// The canvas's end() function will return true if something
	// was updated and the backbuffer needs to be swapped. Once
	// the buffers are swapped, also move to the associated canvas.

	if (current_canvas->end()) {
		cout << ".";
		cout.flush();
		// cout << "buffer swap @ " << std::chrono::system_clock::now().time_since_epoch().count() << endl;
		glfwSwapBuffers(window);
		swap_buffer_index = !swap_buffer_index;
	}
}

// Called once per frame; if the 'wanted' cursor differs from the currently 'active' one
// then update it. The 'wanted' cursor must be re-set every frame or it will default
// back to the standard cursor.
void update_cursor(GLFWwindow *window) {
	if (cur_wanted != cur_active) {
		glfwSetCursor(window, cur_wanted);
		cur_active = cur_wanted;
	}
	cur_wanted = 0;
}

void run_window_loop(GLFWwindow *window, NVGcontext *nvgc) {

	// The really low-end AMD card that I test on really really doesn't like v-sync.
	// A swap interval of 1 or -1 seems to cause huge delays when swapping; ~20FPS
	// Seeing as this isn't exactly Crysis, I'll just keep it off
	glfwSwapInterval(0);

	// Notify all canvas's of the initial size of the buffers.

	for (auto &canvas : swap_buffer_canvas) glfwGetWindowSize(window, &current_canvas->state->size.x, &current_canvas->state->size.y);

	while (!glfwWindowShouldClose(window)) {

		// Block until an event comes through, to prevent needless CPU spinning.
		// Around 1% CPU usage when idle on a Raspberry Pi 4 running Gnome.
		// The UI will run at kinda-sorta 50 updates per second when there are no events.
		glfwWaitEventsTimeout(0.02);

		render(window);
		update_cursor(window);
	}
}

// Whenever the window is resized or the contents of the buffers
// might've been damaged, all canvas's should be notified of the new
// size and told to completely redraw the UI.
void window_refresh_callback(GLFWwindow *window) {
	for (auto &canvas : swap_buffer_canvas) canvas.clear = true;
	render(window);
}

void window_size_callback(GLFWwindow *window, int width, int height) {
	current_canvas->state->size = { width, height };
	for (auto &canvas : swap_buffer_canvas) canvas.clear = true;
}

void cursor_location_callback(GLFWwindow *window, double x, double y) {
	current_canvas->state->cursor = { static_cast<int>(x), static_cast<int>(y) };
}

void cursor_enter_callback(GLFWwindow *window, int entered) {
	current_canvas->state->cursor_enabled = entered;
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
	current_canvas->state->mouse_buttons[button] = action;
}

void char_callback(GLFWwindow *window, unsigned int unicode) {

}

int main(int c, char **v) {

	glfwSetErrorCallback(glfw_error_callback);
	exit_with_error_if(glfwInit() == GLFW_FALSE, "There was a problem initializing the GLFW library.", 1);
	glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
	auto window = glfwCreateWindow(800, 600, "GLFW", 0, 0);
	exit_with_error_if(window == nullptr, "There was a problem creating an OpenGL context.", 2);
	glfwSetWindowRefreshCallback(window, window_refresh_callback);
	glfwSetWindowSizeCallback(window, window_size_callback);
	glfwSetCursorPosCallback(window, cursor_location_callback);
	glfwSetCursorEnterCallback(window, cursor_enter_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCharCallback(window, char_callback);
	glfwMakeContextCurrent(window);
	exit_with_error_if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)), "There was a problem determining which OpenGL extensions are present on this system.", 3);
	auto nvgc = nvgCreateGL2(NVG_ANTIALIAS);
	exit_with_error_if(nvgc == nullptr, "There was a problem initializing the NanoVG library.", 4);

	// Tell each canvas where to find the NanoVG context. It's fine
	// for all canvas's to utilize the same context.

	swap_buffer_canvas[0].state = std::make_shared<cgtb::ui::canvas::shared_state>();
	swap_buffer_canvas[0].state->nvgc = nvgc;
	swap_buffer_canvas[1].state = swap_buffer_canvas[0].state;

	run_window_loop(window, nvgc);

	glfwTerminate();

	return 0;
}

#ifdef WIN32_LEAN_AND_MEAN

int WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR cmdline, int show_cmd) {
	return main(__argc, __argv);
}

#endif