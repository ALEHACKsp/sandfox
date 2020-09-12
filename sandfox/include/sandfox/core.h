#pragma once

#include <memory>
#include <functional>
#include <spdlog/spdlog.h>
#include <sandfox/ui.h>

namespace sandfox::core {

	extern std::shared_ptr<spdlog::logger> logger;
	extern std::function<bool(void)> on_init;
	extern std::function<bool(void)> on_update;
	extern std::function<void(void)> on_shutdown;
	extern std::shared_ptr<ui::canvas> canvas;

	bool init();
	bool update();
	void shutdown();
}