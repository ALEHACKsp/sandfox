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

GLFWwindow *sf_window = 0;
NVGcontext *sf_nvg = 0;

//impl.cpp
NVGcontext *sf_impl_nvg_create();
void sf_impl_nvg_destroy(NVGcontext *);

bool sandfox::core::init() {
	if (!core::logger) {
		core::logger = spdlog::stdout_color_mt("Core");
		core::logger->set_level(spdlog::level::debug);
	}
	glfwSetErrorCallback([](int code, const char *what) { core::logger->error(what); });
	if (!sf_window) {
		if (glfwInit() != GLFW_TRUE) {
			core::logger->error("GLFW initialization failure.");
			core::shutdown();
			return false;
		} else core::logger->debug("GLFW loaded fine.");
		glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
		if (core::on_init) glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		sf_window = glfwCreateWindow(640, 480, "Sandfox", 0, 0);
		if (!sf_window) {
			core::shutdown();
			return false;
		}  else core::logger->debug("Created a GLFW window.");
	}
	glfwSetFramebufferSizeCallback(sf_window, [](GLFWwindow *, int w, int h) {
		glViewport(0, 0, w, h);
		if (core::canvas && core::canvas->state) {
			core::canvas->state->size = { w, h };
			core::canvas->dirty.push_back({ 0, { { 0, 0 }, { w, h } } });
		}
	});
	glfwSetWindowRefreshCallback(sf_window, [](GLFWwindow *) {
		if (core::canvas && core::canvas->state) core::canvas->dirty.push_back({ 0, { { 0, 0 }, { core::canvas->state->size.x, core::canvas->state->size.y } } });
	});
	glfwSetCursorPosCallback(sf_window, [](GLFWwindow *, double x, double y) {
		if (core::canvas && core::canvas->state) core::canvas->state->cur = { x, y };
	});
	glfwSetCursorEnterCallback(sf_window, [](GLFWwindow *, int entered) {
		if (core::canvas && core::canvas->state) core::canvas->state->cursor_enabled = entered;
	});
	glfwMakeContextCurrent(sf_window);
	if (!gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		core::logger->error("GLAD GLES2 load failure.");
		core::shutdown();
		return false;
	} else core::logger->debug("GLAD loaded fine.");
	core::logger->debug("OpenGL: {} ({})", glGetString(GL_VERSION), glGetString(GL_RENDERER));
	core::logger->debug("Shading Language: {}", glGetString(GL_SHADING_LANGUAGE_VERSION));
	if (!sf_nvg) {
		sf_nvg = sf_impl_nvg_create();
		if (!sf_nvg) {
			core::logger->error("NanoVG initialization error.");
			core::shutdown();
			return false;
		} else core::logger->debug("Created a NanoVG context.");
	}
	if (core::on_init) {
		if (core::on_init()) glfwShowWindow(sf_window);
		else {
			core::shutdown();
			return false;
		}
	}
	if (!core::canvas) {
		int w, h;
		glfwGetFramebufferSize(sf_window, &w, &h);
		core::canvas = std::make_shared<ui::canvas>();
		core::canvas->state = std::make_shared<ui::canvas::shared_state>();
		core::canvas->state->nvgc = sf_nvg;
		core::canvas->state->size = { w, h };
	}
	logger->debug("Initialization completed =^-_-^=");
	return true;
}

bool sandfox::core::update() {
	auto video_mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	glfwWaitEventsTimeout(video_mode ? 1.0 / video_mode->refreshRate : 1.0 / 30.0);
	return (core::on_update ? core::on_update() : true) && (sf_window && !glfwWindowShouldClose(sf_window));
}

void sandfox::core::shutdown() {
	if (sf_nvg) sf_impl_nvg_destroy(sf_nvg);
	if (sf_window) glfwDestroyWindow(sf_window);
	if (logger) logger->debug("Goodbye");
	sf_nvg = 0;
	sf_window = 0;
	core::canvas.reset();
	core::logger.reset();
}