#include <sandfox/core.h>
#include <sandfox/img.h>

#include <nanovg.h>
#include <glad.h>
#include <glm/common.hpp>
#include <filesystem>

namespace sf = sandfox;

const NVGcolor palette_a[] = {
	nvgRGB(46, 44, 47),
	nvgRGB(71, 91, 99),
	nvgRGB(114, 155, 121)
};

void emit_bg() {
	sf::core::canvas->emit(
		"bg", { 0, 0 }, sf::core::canvas->state->size,
		[](sf::ui::element *elm) {
			return sf::ui::nothing;
		},
		[](NVGcontext *ctx, sf::ui::element *elm) {
			nvgBeginPath(ctx);
			nvgRect(ctx, 0, 0, elm->lr.x, elm->lr.y);
			nvgFillColor(ctx, palette_a[1]);
			nvgFill(ctx);
		}
	);
}

void emit_status_bar(glm::vec2 ul, glm::vec2 lr) {
	sf::core::canvas->emit(
		"status_bar", ul, lr,
		[](sf::ui::element *elm) {
			return sf::ui::nothing;
		},
		[](NVGcontext *ctx, sf::ui::element *elm) {
			nvgBeginPath(ctx);
			nvgRoundedRect(ctx, elm->ul.x, elm->ul.y, elm->lr.x - elm->ul.x, elm->lr.y - elm->ul.y, 4);
			nvgFillColor(ctx, palette_a[0]);
			nvgFill(ctx);
		}
	);
}

void emit_main_panel(glm::vec2 ul, glm::vec2 lr) {
	sf::core::canvas->emit(
		"main_panel", ul, lr,
		[](sf::ui::element *elm) {
			return sf::ui::nothing;
		},
		[](NVGcontext *ctx, sf::ui::element *elm) {
			nvgBeginPath(ctx);
			nvgRoundedRect(ctx, elm->ul.x, elm->ul.y, elm->lr.x - elm->ul.x, elm->lr.y - elm->ul.y, 4);
			nvgFillColor(ctx, palette_a[0]);
			nvgFill(ctx);
		}
	);
}

void render_tab_a_content(glm::vec2 ul, glm::vec2 lr) {
	sf::core::canvas->emit(
		"tab_a_content", ul, lr,
		[](sf::ui::element *elm) {
			return sf::ui::nothing;
		},
		[&](NVGcontext *ctx, sf::ui::element *elm) {
			const glm::vec2 center = {
				glm::round(elm->ul.x + ((elm->lr.x - elm->ul.x) * 0.5f)),
				glm::round(elm->ul.y + ((elm->lr.y - elm->ul.y) * 0.5f))
			};
			nvgBeginPath(ctx);
			nvgRoundedRect(ctx, elm->ul.x, elm->ul.y, elm->lr.x - elm->ul.x, elm->lr.y - elm->ul.y, 4);
			nvgFillColor(ctx, nvgRGBA(16, 16, 16, 128));
			nvgFill(ctx);
			nvgBeginPath(ctx);
			const float radius = glm::round(glm::min(elm->lr.x - elm->ul.x, elm->lr.y - elm->ul.y) * 0.4f);
			nvgCircle(ctx, center.x, center.y, radius);
			nvgStrokeColor(ctx, nvgRGB(192, 192, 192));
			nvgStrokeWidth(ctx, 3.0f);
			nvgStroke(ctx);
		}
	);
	sf::img::emit(
		sf::core::canvas.get(),
		"testing-icon-thingy",
		{ "icons/svg/bootstrap/alarm.svg", { 16, 16 } },
		{ ul.x + 20, lr.y - 20},
		{ 16, 16 }
	);
}

void render_content() {
	const float padding = 6.0f;
	emit_bg();
	glm::vec2 ul, lr;
	ul = { padding, padding };
	lr = { sf::core::canvas->state->size.x - padding, ul.y + 20 };
	emit_status_bar(ul, lr);
	ul.y = lr.y + padding;
	lr.y = sf::core::canvas->state->size.y - padding;
	emit_main_panel(ul, lr);
	render_tab_a_content(ul + padding * 2.0f, lr - padding * 2.0f);
}

bool load_fonts() {
	if (!std::filesystem::exists("fonts")) {
		spdlog::error("Unable to locate fonts.");
		return false;
	}
	for (auto &file : std::filesystem::directory_iterator("fonts")) {
		if (!file.is_regular_file()) continue;
		auto name = file.path().stem().string();
		int r = nvgCreateFont(sf::core::nvg, name.data(), file.path().string().data());
		if (r >= 0) sf::core::logger->debug("Loaded font \"{}\" to index #{}. ({})", name, r, file.path().string());
		else sf::core::logger->debug("Unable to load font from file: {}", file.path().string());
	}
	return true;
}

void process_new_images(std::filesystem::path path, std::shared_ptr<std::vector<char>> data) {
	if (path.string().rfind("icons/") == std::string::npos) return;
	for (int p = 0; p < data->size(); p += 4) {
		for (int i = 0; i < 3; i++) {
			(*data)[p + i] = 255 - (*data)[p + i];
		}
	}
}

int main() {
	std::atexit(sf::core::shutdown);
	sf::img::on_fs_image_data = process_new_images;
	sf::core::on_init = []() {
		return load_fonts();
	};
	sf::core::on_update = []() {
		sf::core::canvas->begin();
		render_content();
		if (sf::core::canvas->end()) glFinish();
		return true;
	};
	if (sf::core::init()) {
		while (sf::core::update());
	}
	return 0;
}