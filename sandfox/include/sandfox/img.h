#pragma once

#include <sandfox/ui.h>
#include <glm/vec2.hpp>
#include <string>
#include <string_view>
#include <functional>
#include <filesystem>
#include <optional>
#include <tuple>
#include <memory>

namespace sandfox::img {

	using id = std::tuple<std::string, glm::ivec2>;

	extern std::function<void(std::filesystem::path, std::shared_ptr<std::vector<char>>)> on_new_image_data;

	std::optional<std::shared_ptr<int>> get(const id &key);
	std::optional<std::shared_ptr<int>> find(const id &key);

	void emit(ui::canvas *to, const std::string_view &uuid, const id &key, const glm::vec2 &center, const glm::vec2 &size);
}