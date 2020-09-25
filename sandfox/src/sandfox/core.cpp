#include <sandfox/core.h>
#include <sandfox/ui.h>

#include <spdlog/sinks/stdout_color_sinks.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad.h>
#include <nanovg.h>
#include <uv.h>

std::shared_ptr<spdlog::logger> sandfox::core::logger;
std::string sandfox::core::default_logging_pattern = "%C%m%d %H%M [%^%L %n%$] %v";
std::function<bool(void)> sandfox::core::on_init;
std::function<bool(void)> sandfox::core::on_update;
std::function<void(void)> sandfox::core::on_shutdown;
std::shared_ptr<sandfox::ui::canvas> sandfox::core::canvas;

GLFWwindow *sandfox::core::window = 0;
NVGcontext *sandfox::core::nvg = 0;

sandfox::core::cursor sandfox::core::active_cursor = sandfox::core::cursor::normal;

namespace sandfox::core {
	cursor applied_cursor = active_cursor;
}

//impl.cpp
NVGcontext *sf_impl_nvg_create();
void sf_impl_nvg_destroy(NVGcontext *);

bool sandfox::core::init() {
	if (!core::logger) {
		core::logger = spdlog::stdout_color_mt("Sandfox");
		core::logger->set_level(spdlog::level::debug);
		core::logger->set_pattern(core::default_logging_pattern);
	}
	glfwSetErrorCallback([](int code, const char *what) { core::logger->error(what); });
	if (!core::window) {
		if (glfwInit() != GLFW_TRUE) {
			core::logger->error("GLFW initialization failure.");
			core::shutdown();
			return false;
		} else core::logger->debug("GLFW loaded fine.");
		glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
		if (core::on_init) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		core::window = glfwCreateWindow(640, 480, "Sandfox", 0, 0);
		if (!core::window) {
			core::shutdown();
			return false;
		}  else core::logger->debug("Created a GLFW window.");
	}
	glfwSetFramebufferSizeCallback(core::window, [](GLFWwindow *, int w, int h) {
		glViewport(0, 0, w, h);
		if (core::canvas && core::canvas->state) {
			core::canvas->state->size = { w, h };
			core::canvas->dirty.push_back({ 0, { { 0, 0 }, { w, h } } });
		}
	});
	glfwSetWindowRefreshCallback(core::window, [](GLFWwindow *) {
		if (core::canvas && core::canvas->state) {
			core::canvas->dirty.push_back({ 0, { { 0, 0 }, { core::canvas->state->size.x, core::canvas->state->size.y } } });
			#ifdef _WIN32
			if (core::on_update) core::on_update();
			#endif
		}
	});
	glfwSetCursorPosCallback(core::window, [](GLFWwindow *, double x, double y) {
		if (core::canvas && core::canvas->state) core::canvas->state->cur = { x, y };
	});
	glfwSetCursorEnterCallback(core::window, [](GLFWwindow *, int entered) {
		if (core::canvas && core::canvas->state) core::canvas->state->cursor_enabled = entered;
	});
	glfwSetMouseButtonCallback(core::window, [](GLFWwindow *, int button, int action, int mods) {
		if (core::canvas && core::canvas->state) core::canvas->state->mouse_buttons[button] = action;
	});
	glfwMakeContextCurrent(core::window);
	#ifndef __EMSCRIPTEN__
	if (!gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		core::logger->error("GLAD GLES2 load failure.");
		core::shutdown();
		return false;
	} else core::logger->debug("GLAD loaded fine.");
	#endif
	core::logger->debug("OpenGL: {} ({})", glGetString(GL_VERSION), glGetString(GL_RENDERER));
	core::logger->debug("Shading Language: {}", glGetString(GL_SHADING_LANGUAGE_VERSION));
	if (!core::nvg) {
		core::nvg = sf_impl_nvg_create();
		if (!core::nvg) {
			core::logger->error("NanoVG initialization error.");
			core::shutdown();
			return false;
		} else core::logger->debug("Created a NanoVG context.");
	}
	if (!core::canvas) {
		int w, h;
		glfwGetFramebufferSize(core::window, &w, &h);
		core::canvas = std::make_shared<ui::canvas>();
		core::canvas->state = std::make_shared<ui::canvas::shared_state>();
		core::canvas->state->nvgc = core::nvg;
		core::canvas->state->size = { w, h };
	}
	if (core::on_init) {
		if (core::on_init()) glfwShowWindow(core::window);
		else {
			core::shutdown();
			return false;
		}
	}
	logger->debug("Initialization completed =^-_-^=");
	return true;
}

bool sandfox::core::update() {
	uv_run(uv_default_loop(), UV_RUN_NOWAIT);
	auto video_mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	glfwWaitEventsTimeout(video_mode ? 1.0 / video_mode->refreshRate : 1.0 / 30.0);
	active_cursor = cursor::normal;
	bool keep_running = (core::on_update ? core::on_update() : true) && (core::window && !glfwWindowShouldClose(core::window));
	if (!keep_running) return false;
	if (active_cursor != applied_cursor) {
		static GLFWcursor *glfw_cursor_normal = 0;
		static GLFWcursor *glfw_cursor_beam = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
		static GLFWcursor *glfw_cursor_cross = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
		static GLFWcursor *glfw_cursor_hand = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
		static GLFWcursor *glfw_cursor_h_res = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
		static GLFWcursor *glfw_cursor_v_res = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
		switch (active_cursor) {
			case cursor::normal: glfwSetCursor(core::window, glfw_cursor_normal); break;
			case cursor::beam: glfwSetCursor(core::window, glfw_cursor_beam); break;
			case cursor::cross: glfwSetCursor(core::window, glfw_cursor_cross); break;
			case cursor::hand: glfwSetCursor(core::window, glfw_cursor_hand); break;
			case cursor::h_res: glfwSetCursor(core::window, glfw_cursor_h_res); break;
			case cursor::v_res: glfwSetCursor(core::window, glfw_cursor_v_res); break;
		}
		applied_cursor = active_cursor;
		core::logger->debug("Updated cursor: {}", static_cast<int>(applied_cursor));
	}
	return true;
}

static void sf_wait_for_uv() {
	if (int initial = uv_run(uv_default_loop(), UV_RUN_NOWAIT); initial) {
		uv_walk(uv_default_loop(), [](uv_handle_t *handle, void *arg) {
			if (uv_is_closing(handle) == 0) {
				if (sandfox::core::logger) sandfox::core::logger->warn("Handle {} was left open; it will be closed.", reinterpret_cast<void *>(handle));
				uv_close(handle, [](uv_handle_t *handle) {
					if (sandfox::core::logger) sandfox::core::logger->debug("Closed handle: {}", reinterpret_cast<void *>(handle));
				});
			}
		}, 0);
		if (sandfox::core::logger) sandfox::core::logger->debug("Waiting for UV to finish.");
		while (uv_run(uv_default_loop(), UV_RUN_DEFAULT) != 0);
	} else if (sandfox::core::logger) sandfox::core::logger->debug("UV is clean. No handles open.");
}

void sandfox::core::shutdown() {
	if (core::window) glfwHideWindow(core::window);
	sf_wait_for_uv();
	if (core::nvg) sf_impl_nvg_destroy(core::nvg);
	if (core::window) glfwDestroyWindow(core::window);
	glfwTerminate();
	if (logger) logger->debug("Goodbye");
	core::nvg = 0;
	core::window = 0;
	core::canvas.reset();
	core::logger.reset();
}