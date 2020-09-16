#include <sandfox/core.h>
#include <sandfox/ui.h>

#include <spdlog/sinks/stdout_color_sinks.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad.h>
#include <nanovg.h>
#include <uv.h>

std::shared_ptr<spdlog::logger> sandfox::core::logger;
std::function<bool(void)> sandfox::core::on_init;
std::function<bool(void)> sandfox::core::on_update;
std::function<void(void)> sandfox::core::on_shutdown;
std::shared_ptr<sandfox::ui::canvas> sandfox::core::canvas;

GLFWwindow *sandfox::core::window = 0;
NVGcontext *sandfox::core::nvg = 0;

//impl.cpp
NVGcontext *sf_impl_nvg_create();
void sf_impl_nvg_destroy(NVGcontext *);

bool sandfox::core::init() {
	if (!core::logger) {
		core::logger = spdlog::stdout_color_mt("Core");
		core::logger->set_level(spdlog::level::debug);
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
		if (core::canvas && core::canvas->state) core::canvas->dirty.push_back({ 0, { { 0, 0 }, { core::canvas->state->size.x, core::canvas->state->size.y } } });
	});
	glfwSetCursorPosCallback(core::window, [](GLFWwindow *, double x, double y) {
		if (core::canvas && core::canvas->state) core::canvas->state->cur = { x, y };
	});
	glfwSetCursorEnterCallback(core::window, [](GLFWwindow *, int entered) {
		if (core::canvas && core::canvas->state) core::canvas->state->cursor_enabled = entered;
	});
	glfwMakeContextCurrent(core::window);
	if (!gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		core::logger->error("GLAD GLES2 load failure.");
		core::shutdown();
		return false;
	} else core::logger->debug("GLAD loaded fine.");
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
	auto video_mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	glfwWaitEventsTimeout(video_mode ? 1.0 / video_mode->refreshRate : 1.0 / 30.0);
	return (core::on_update ? core::on_update() : true) && (core::window && !glfwWindowShouldClose(core::window));
}

void sandfox::core::shutdown() {
	if (core::nvg) sf_impl_nvg_destroy(core::nvg);
	if (core::window) glfwDestroyWindow(core::window);
	if (logger) logger->debug("Goodbye");
	core::nvg = 0;
	core::window = 0;
	core::canvas.reset();
	core::logger.reset();
}