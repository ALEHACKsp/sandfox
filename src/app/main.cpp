#include <iostream>
#include <sstream>
#include <thread>
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

// Since we're not rendering to a texture, but right to the backbuffer
// we have to handle double-buffering.

// Due to double-buffering, two separate UI canvas's need to be managed.
// This ensures that both buffers are individually updated as needed.

// In the case of triple-buffering, three separate UI canvas's would be needed.

int swap_buffer_index = 0;

cgtb::ui::canvas *ui = 0;
cgtb::ui::canvas swap_buffer_canvas[2];

GLFWcursor *cur_active = 0, *cur_wanted = 0, *cur_normal = 0, *cur_select = 0;

std::map<std::string, std::map<NVGcontext *, int>> nvg_icons;

int make_icon(NVGcontext *context, const std::string_view &name) {
	auto existing_texture_it = nvg_icons.find(name.data());
	if (existing_texture_it != nvg_icons.end()) return existing_texture_it->second[context];
	auto image_data_it = cgtb::icons.find(name.data());
	if (image_data_it == cgtb::icons.end()) return 0;
	int new_image = nvgCreateImageMem(context, 0, image_data_it->second.first, image_data_it->second.second);
	if (!new_image) return 0;
	nvg_icons[name.data()][context] = new_image;
	return new_image;
}

void show_error_message(const std::string_view &what) {
	#if defined(WIN32_LEAN_AND_MEAN) && !defined(__EMSCRIPTEN__)
	MessageBoxA(NULL, what.data(), "Error", MB_ICONERROR);
	#else
	std::cerr << what.data() << std::endl;
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

class fancy_button : public cgtb::ui::wrapper {

	cgtb::ui::canvas::action poll_cb(const cgtb::ui::element &element) override {
		return cgtb::ui::canvas::nothing;
	}

	void render_cb(NVGcontext *nvgc, const cgtb::ui::element &element) override {
	
		nvgBeginPath(nvgc);
		nvgRect(
			nvgc,
			element.body.x1, element.body.y1,
			element.body.x2 - element.body.x1,
			element.body.y2 - element.body.y1
		);
		nvgFillColor(nvgc, nvgRGB(255, 0, 0));
		nvgFill(nvgc);
	}

	public:

	fancy_button(const std::string_view &uuid) : wrapper(uuid) {

	}
};

// auto the_button = std::make_unique<fancy_button>("the_one_to_rull_them_all");

void render(GLFWwindow *window) {

	// Update the canvas pointer to reflect the canvas that's associated
	// with the current swap buffer.

	ui = &swap_buffer_canvas[swap_buffer_index];

	ui->begin();

	ui->emit(
		
		"bg_tile",
		{ 0, 0, ui->size.x, ui->size.y },
		
		[](const cgtb::ui::element &element) {
			return cgtb::ui::canvas::nothing;
		},

		[](NVGcontext *nvgc, const cgtb::ui::element &element) {
			nvgBeginPath(nvgc);
			nvgRect(
				nvgc,
				element.body.x1, element.body.y1,
				element.body.x2 - element.body.x1,
				element.body.y2 - element.body.y1
			);
			nvgFillColor(nvgc, nvgRGB(32, 32, 32));
			nvgFill(nvgc);
		}
	);

	for (int i = 0; i < 10; i++) {

		ui->emit(

			fmt::format("button#{}", i),
			{ 20, 20 + (42 * i), 220, 60 + (42 * i) },

			[](const cgtb::ui::element &element) {
				if (element.hover) cur_wanted = cur_select;
				if (!element.previous) return cgtb::ui::canvas::nothing;
				if ((element.hover || element.previous->hover) && element.cursor != element.previous->cursor) return cgtb::ui::canvas::redraw;
				return cgtb::ui::canvas::nothing;
			},

			[&](NVGcontext *nvgc, const cgtb::ui::element &element) {
	
				nvgBeginPath(nvgc);
				nvgRect(
					nvgc,
					element.body.x1, element.body.y1,
					element.body.x2 - element.body.x1,
					element.body.y2 - element.body.y1
				);

				nvgFillColor(nvgc, nvgRGB(92, 92, 92));
				nvgFill(nvgc);

				if (element.hover) {

					auto paint = nvgRadialGradient(
						nvgc,
						element.body.x1 + element.cursor.x,
						element.body.y1 + element.cursor.y,
						20,
						120,
						nvgRGBA(255, 255, 255, 128),
						nvgRGBA(255, 255, 255, 0)
					);

					nvgStrokePaint(nvgc, paint);
					nvgStrokeWidth(nvgc, 2);
					nvgStroke(nvgc);
				}
			}
		);
	}

	for (int side_bar_icon_index = 0; side_bar_icon_index < 6; side_bar_icon_index++) {

		ui->emit(

			fmt::format("side_icon_{}", side_bar_icon_index),
			{ ui->size.x - 34, (ui->size.y - 34) - (24 * side_bar_icon_index), ui->size.x - 10, (ui->size.y - 10) - (24 * side_bar_icon_index) },

			[](const cgtb::ui::element &element) {
				if (element.hover) cur_wanted = cur_select;
				if (!element.previous) return cgtb::ui::canvas::nothing;
				if (static_cast<bool>(element.hover) != static_cast<bool>(element.previous->hover)) return cgtb::ui::canvas::redraw_deep;
				return cgtb::ui::canvas::nothing;
			},

			[](NVGcontext *nvgc, const cgtb::ui::element &element) {
			
				nvgBeginPath(nvgc);

				nvgRect(
					nvgc,
					element.body.x1, element.body.y1,
					element.body.x2 - element.body.x1,
					element.body.y2 - element.body.y1
				);

				int icon = make_icon(nvgc, element.hover ? "24.material.sentiment_very_satisfied" : "24.material.sentiment_very_dissatisfied");
				auto paint = nvgImagePattern(nvgc, element.body.x1, element.body.y1, 24, 24, 0, icon, 1);

				paint.innerColor = element.hover ? nvgRGBA(128, 255, 128, 255) : nvgRGBA(255, 128, 128, 128);

				nvgFillPaint(nvgc, paint);
				nvgFill(nvgc);
			}

		);
	}

	ui->emit(

		"big_icon",
		{ 10, ui->size.y - 58, 58, ui->size.y - 10 },

		[](const cgtb::ui::element &element) {
			return cgtb::ui::canvas::nothing;
		},

		[](NVGcontext *nvgc, const cgtb::ui::element &element) {
		
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

	// The canvas's end() function will return true if something
	// was updated and the backbuffer needs to be swapped. Once
	// the buffers are swapped, also move to the associated canvas.

	if (ui->end()) {
		std::cout << "buffer swap @ " << std::chrono::system_clock::now().time_since_epoch().count() << std::endl;
		glfwSwapBuffers(window);
		swap_buffer_index = !swap_buffer_index;
		// emit_render_indicator = true;
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

	for (auto &canvas : swap_buffer_canvas) glfwGetWindowSize(window, &canvas.size.x, &canvas.size.y);

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
	for (auto &canvas : swap_buffer_canvas) {
		canvas.size = { width, height };
		canvas.clear = true;
	}
}

void cursor_location_callback(GLFWwindow *window, double x, double y) {
	for (auto &canvas : swap_buffer_canvas) canvas.cursor = { x, y };
}

void cursor_enter_callback(GLFWwindow *window, int entered) {
	for (auto &canvas : swap_buffer_canvas) canvas.cursor_enabled = entered > 0;
}

int main(int c, char **v) {

	glfwSetErrorCallback(glfw_error_callback);
	exit_with_error_if(glfwInit() == GLFW_FALSE, "There was a problem initializing the GLFW library.", 1);
	auto window = glfwCreateWindow(800, 600, "GLFW", 0, 0);

	cur_normal = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	cur_select = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

	exit_with_error_if(window == nullptr, "There was a problem creating an OpenGL context.", 2);
	glfwSetWindowRefreshCallback(window, window_refresh_callback);
	glfwSetWindowSizeCallback(window, window_size_callback);
	glfwSetCursorPosCallback(window, cursor_location_callback);
	glfwSetCursorEnterCallback(window, cursor_enter_callback);
	glfwMakeContextCurrent(window);
	exit_with_error_if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)), "There was a problem determining which OpenGL extensions are present on this system.", 3);
	auto nvgc = nvgCreateGL2(NVG_ANTIALIAS);
	exit_with_error_if(nvgc == nullptr, "There was a problem initializing the NanoVG library.", 4);

	// Tell each canvas where to find the NanoVG context. It's fine
	// for all canvas's to utilize the same context.

	for (auto &canvas : swap_buffer_canvas) canvas.nvgc = nvgc;

	run_window_loop(window, nvgc);

	glfwTerminate();

	return 0;
}

#ifdef WIN32_LEAN_AND_MEAN

int WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR cmdline, int show_cmd) {
	return main(__argc, __argv);
}

#endif