#pragma once

#include <memory>
#include <functional>
#include <spdlog/logger.h>
#include <sandfox/ui.h>

struct GLFWwindow;
struct NVGcontext;

namespace sandfox::core {

	extern GLFWwindow *window;
	extern NVGcontext *nvg;

	extern std::shared_ptr<spdlog::logger> logger;
	extern std::string default_logging_pattern;

	extern std::function<bool(void)> on_init;
	extern std::function<bool(void)> on_update;
	extern std::function<void(void)> on_shutdown;
	extern std::shared_ptr<ui::canvas> canvas;

	bool init();
	bool update();
	void shutdown();
}