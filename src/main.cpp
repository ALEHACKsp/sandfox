#include <sandfox/core.h>
#include <nanovg.h>
#include <glad.h>

namespace sf = sandfox;

void emit_bg() {
	sf::core::canvas->emit(
		"bg", { 0, 0 }, sf::core::canvas->state->size,
		[](sf::ui::element *elm) {
			return sf::ui::nothing;
		},
		[](NVGcontext *ctx, sf::ui::element *elm) {
			nvgBeginPath(ctx);
			nvgRect(ctx, 0, 0, elm->lr.x, elm->lr.y);
			nvgFillColor(ctx, nvgRGB(255, 128, 64));
			nvgFill(ctx);
		}
	);
}

void emit_rect() {
	sf::core::canvas->emit(
		"rect", { 20, 120 }, { 120, 320 },
		[](sf::ui::element *elm) {
			return elm->hover ? sf::ui::redraw : sf::ui::nothing;
		},
		[](NVGcontext *ctx, sf::ui::element *elm) {
			nvgBeginPath(ctx);
			nvgRect(ctx, elm->ul.x, elm->ul.y, elm->lr.x - elm->ul.x, elm->lr.y - elm->ul.y);
			nvgFillColor(ctx, nvgRGB(32, 32, 32));
			nvgFill(ctx);
			nvgBeginPath(ctx);
			nvgCircle(ctx, sf::core::canvas->state->cur.x, sf::core::canvas->state->cur.y, 10);
			nvgFillColor(ctx, nvgRGB(255, 0, 0));
			nvgFill(ctx);
		}
	);
}

int main() {

	std::atexit(sf::core::shutdown);

	sf::core::on_init = []() {
		sf::core::logger->info("Hi -_-");
		return true;
	};

	sf::core::on_update = []() {

		sf::core::canvas->begin();

		emit_bg();
		emit_rect();
	
		if (sf::core::canvas->end()) {
			spdlog::info("Redraw");
			glFinish();
		}
	
		return true;
	};

	if (sf::core::init()) while (sf::core::update());

	return 0;
}