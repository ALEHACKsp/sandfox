#include <iostream>
#include <sstream>
#include <thread>
#include <array>
#include <map>

#include <cgtb/third-party/glad.h>
#include <cgtb/third-party/nanovg.h>
#define NANOVG_GL2_IMPLEMENTATION
#include <cgtb/third-party/nanovg_gl.h>
#include <cgtb/third-party/stb_image.h>
#include <cgtb/third-party/stb_image_resize.h>
#include <GLFW/glfw3.h>

#if defined(_WIN32) || defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <cgtb/ui/canvas.h>
#include <cgtb/ui/element.h>
#include <cgtb/ui/reactor.h>
#include <fmt/core.h>
#include "emico/include/emico.h"

using fmt::format;
using std::cout;
using std::endl;

using namespace std::placeholders;

cgtb::ui::canvas main_canvas;

GLFWcursor *cur_active = 0, *cur_wanted = 0, *cur_normal = 0, *cur_select = 0;

// Previously loaded icon textures. Used for lookups so the same image isn't loaded twice.
// gl_icon_textures[NAME][{ WIDTH, HEIGHT }] -> OpenGL texture ID
std::map<std::string, std::map<std::pair<int, int>, int>> gl_icon_textures;

int get_icon(NVGcontext *context, const std::string_view &name, const int &resize_to_w = 0, const int &resize_to_h = 0) {

	auto existing_texture_it = gl_icon_textures.find(name.data());

	// Check if the texture has already been loaded and return it if it has.

	if (existing_texture_it != gl_icon_textures.end()) {
		auto specific_size = existing_texture_it->second.find({ resize_to_w, resize_to_h });
		if (specific_size != existing_texture_it->second.end()) return specific_size->second;
	}

	// Search the embedded icons for an entry with that name.

	auto full_name = format("icon.{}.png", name);
	auto entry = emico::assets.find(full_name);

	// Return 0 if there aren't any matching icons.

	if (entry == emico::assets.end()) {
		cout << "Icon not found: " << full_name << endl;
		return 0;
	}

	cout << "Found icon '" << full_name << "'. (" << entry->second.second << " bytes) @ " << entry->second.first << endl;

	// Load the image data and get a pointer to an RGBA byte array in response.

	int w, h, n;
	auto rgba = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(entry->second.first), entry->second.second, &w, &h, &n, 4);

	// Return 0 if there was something wrong with the data.

	if (!rgba) {
		cout << "Bad data. #1" << endl;
		return 0;
	}

	// Invert the RGB values.
	// Turning the icon from black to white.

	for (int p = 0; p < w * h * 4; p += 4) {
		rgba[p] = 255 - rgba[p];
		rgba[p + 1] = 255 - rgba[p + 1];
		rgba[p + 2] = 255 - rgba[p + 2];
	}
	
	int new_image = 0;

	if (resize_to_w && resize_to_h && (w != resize_to_w || h != resize_to_h)) {
		cout << "Resizing icon to preferred dimensions..." << endl;
		std::vector<stbi_uc> scaled_image_rgba(resize_to_w * resize_to_h * 4);
		if (stbir_resize_uint8(rgba, w, h, 0, scaled_image_rgba.data(), resize_to_w, resize_to_h, 0, 4)) {
			w = resize_to_w;
			h = resize_to_h;
			new_image = nvgCreateImageRGBA(context, w, h, 0, scaled_image_rgba.data());
		} else cout << "Failure during resize operation." << endl;
	}

	// The image doesn't need to be resized, or the resize operation failed.
	// So, just try to load it without resizing it.

	if (!new_image) new_image = nvgCreateImageRGBA(context, w, h, 0, rgba);

	// We're done using the 'rgba' pointer now.

	stbi_image_free(rgba);

	// If 'new_image' is still 0 then there was probably an issue with available video memory.

	if (!new_image) {
		cout << "Bad data. #2" << endl;
		return 0;
	}

	// Add the newly generate image / texture to the cache so we can look it up later.

	gl_icon_textures[name.data()][{ w, h }] = new_image;

	cout << "New icon '" << name << "'; GL texture #" << new_image << " (" << w << " by " << h << ")" << endl;

	return new_image;
}

std::map<std::string, int> nvg_fonts;

int get_font(NVGcontext *context, const std::string_view &name) {

	auto existing_font_it = nvg_fonts.find(name.data());

	if (existing_font_it != nvg_fonts.end()) return existing_font_it->second;

	auto full_name = format("font.{}.ttf", name);
	auto entry = emico::assets.find(full_name);

	if (entry == emico::assets.end()) {
		cout << "Unable to locate embedded font:" << full_name << endl;
		return 0;
	}

	int new_font = nvgCreateFontMem(context, name.data(), reinterpret_cast<const unsigned char *>(entry->second.first), entry->second.second, 0);

	if (new_font == -1) {
		cout << "Encountered an error while trying to process font data." << endl;
		return -1;
	}

	cout << "New font \"" << full_name << "\" created; NVG item #" << new_font << endl;

	nvg_fonts[name.data()] = new_font;

	return new_font;
}

// This will show an error message box on Windows but will just print to std::cerr everwhere else.
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

bool render(GLFWwindow *window) {

	main_canvas.begin();

	main_canvas.emit(
		
		"bg_tile",

		{
			0,
			0,
			main_canvas.state->size.x,
			main_canvas.state->size.y
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
			nvgFillColor(nvgc, nvgRGB(16, 16, 16));
			nvgFill(nvgc);
		}
	);

	struct floating_button : cgtb::ui::reactor {

		const std::string uuid, label;

		cgtb::ui::area body;
		std::string icon_name;

		floating_button(const std::string_view &uuid, const std::string_view &label) : uuid(uuid), label(label) {
			hover_attack = 4;
			hover_decay = 4;
			click_decay[0] = 3;
		}

		cgtb::ui::canvas::action poll(const cgtb::ui::element &element, void *state) {
			auto self = reinterpret_cast<floating_button *>(state);
			self->update(element);
			if (element.hover) cur_wanted = cur_select;
			if (self->click[0].size() || (self->hover != 0 && self->hover != 1) || (element.hover && element.previous && element.cursor != element.previous->cursor)) return cgtb::ui::canvas::redraw_deep;
			return cgtb::ui::canvas::nothing;
		}

		void render(NVGcontext *nvgc, const cgtb::ui::element &element, void *state) {

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

		void emit(cgtb::ui::canvas &canvas) {

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

	buttons[0].emit(main_canvas);
	buttons[1].emit(main_canvas);
	buttons[2].emit(main_canvas);

	// The canvas's end() function will return true if something was updated.

	if (main_canvas.end()) {
		glFinish();
		cout << '.';
		return true;
	} else return false;
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

	// Notify all canvas's of the initial size of the buffers.
	glfwGetWindowSize(window, &main_canvas.state->size.x, &main_canvas.state->size.y);

	while (!glfwWindowShouldClose(window)) {

		// Block until an event comes through, to prevent needless CPU spinning.

		glfwWaitEventsTimeout(render(window) ? 0.02 : 0.008);

		update_cursor(window);
	}
}

// Whenever the window is resized or the contents of the buffers
// might've been damaged, all canvas's should be notified of the new
// size and told to completely redraw the UI.
void window_refresh_callback(GLFWwindow *window) {
	main_canvas.clear = true;
	main_canvas.dirty.push_back({ 0, { 0, 0, main_canvas.state->size.x, main_canvas.state->size.y } });
	render(window);
}

void window_size_callback(GLFWwindow *window, int width, int height) {
	main_canvas.state->size = { width, height };
	main_canvas.clear = true;
}

void cursor_location_callback(GLFWwindow *window, double x, double y) {
	main_canvas.state->cursor = { static_cast<int>(x), static_cast<int>(y) };
}

void cursor_enter_callback(GLFWwindow *window, int entered) {
	main_canvas.state->cursor_enabled = entered;
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
	main_canvas.state->mouse_buttons[button] = action;
}

void char_callback(GLFWwindow *window, unsigned int unicode) {

}

int main(int c, char **v) {

	glfwSetErrorCallback(glfw_error_callback);
	exit_with_error_if(glfwInit() == GLFW_FALSE, "There was a problem initializing the GLFW library.", 1);
	
	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);

	auto window = glfwCreateWindow(800, 600, "GLFW", 0, 0);
	exit_with_error_if(window == nullptr, "There was a problem creating an OpenGL context.", 2);
	std::atexit(glfwTerminate);

	cur_normal = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	cur_select = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

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

	main_canvas.state = std::make_shared<cgtb::ui::canvas::shared_state>();
	main_canvas.state->nvgc = nvgc;

	run_window_loop(window, nvgc);

	return 0;
}

#ifdef WIN32_LEAN_AND_MEAN

int WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR cmdline, int show_cmd) {
	return main(__argc, __argv);
}

#endif