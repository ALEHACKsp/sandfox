#include <sandfox/core.h>
#include <sandfox/img.h>
#include <sandfox/net.h>

#include <spdlog/sinks/stdout_color_sinks.h>

#include <nanovg.h>
#include <glad.h>
#include <glm/common.hpp>

#include <filesystem>
#include <sstream>
#include <vector>
#include <map>

namespace sf = sandfox;

static const auto col_bg = nvgRGB(46, 44, 47);
static const auto col_mid = nvgRGB(71, 91, 99);
static const auto col_front = nvgRGB(114, 155, 121);

using fmt::format;

std::vector<std::shared_ptr<sf::net::tcp_stream>> connections;

void emit_material_button(const std::string_view &uuid, const glm::vec2 &ul, const glm::vec2 &lr) {
	sf::core::canvas->emit(
		uuid, ul, lr,
		[](sf::ui::element *elm) {
			if (!elm->data.has_value()) {
				sf::ui::reactor reactor;
				reactor.hover_attack = 24;
				reactor.hover_decay = 24;
				reactor.punch_decay[0] = 5;
				elm->data = reactor;
			}
			auto &react = std::any_cast<sf::ui::reactor &>(elm->data);
			react.update(*elm);
			// if (element.hover) cur_wanted = cur_select;
			if (react.click[0].size() || (react.hover != 0 && react.hover != 1) || (elm->hover && elm->prev && elm->cur != elm->prev->cur)) return sf::ui::redraw;
			return sf::ui::nothing;
		},
		[](NVGcontext *ctx, sf::ui::element *elm) {
			auto &react = std::any_cast<sf::ui::reactor &>(elm->data);
			nvgBeginPath(ctx);
			nvgRect(
				ctx,
				elm->ul.x, elm->ul.y,
				elm->lr.x - elm->ul.x,
				elm->lr.y - elm->ul.y
			);
			float brightness = 160 - (48 * react.hover);
			nvgFillColor(ctx, nvgRGB(brightness, brightness, brightness));
			nvgFill(ctx);
			for (auto &click : react.click[0]) {
				sf::core::logger->critical(">>> {}", click.second);
				float power = (click.second * click.second) / (2.0f * ((click.second * click.second) - click.second) + 1.0f);
				auto click_paint = nvgRadialGradient(
					ctx,
					elm->ul.x + click.first.x,
					elm->ul.y + click.first.y,
					1200 * (1.0f - click.second),
					1200 * (1.0f - click.second),
					nvgRGBA(255, 255, 255, 96.0f * power),
					nvgRGBA(0, 0, 0, 0)
				);
				nvgFillPaint(ctx, click_paint);
				nvgFill(ctx);
			}
			/*
			auto icon = make_icon(ctx, self->icon_name);
			auto paint = nvgImagePattern(ctx, element.body.x1 + 5, element.body.y1 + 5, 24, 24, 0, icon, 1);
			paint.innerColor = nvgRGB(16, 16, 16);
			nvgBeginPath(ctx);
			nvgRect(ctx, element.body.x1 + 5, element.body.y1 + 5, 24, 24);
			nvgFillPaint(ctx, paint);
			nvgFill(ctx);
			*/
		}
	);
}

void emit_bg() {
	sf::core::canvas->emit(
		"bg", { 0, 0 }, sf::core::canvas->state->size,
		[](sf::ui::element *elm) {
			return sf::ui::nothing;
		},
		[](NVGcontext *ctx, sf::ui::element *elm) {
			nvgBeginPath(ctx);
			nvgRect(ctx, 0, 0, elm->lr.x, elm->lr.y);
			nvgFillColor(ctx, col_bg);
			nvgFill(ctx);
		}
	);
}

struct node {
	glm::vec2 location;
	glm::vec2 target_location;
	std::shared_ptr<sf::net::tcp_stream> stream;
	sf::net::tcp_stream::status last_net_state = sf::net::tcp_stream::dead;
};

std::map<std::string, std::shared_ptr<node>> nodes;

void calculate_target_locations() {
	float radial = 0.0f;
	for (auto &pair : nodes) {
		float x = sinf(radial);
		float y = cosf(radial);
		pair.second->target_location = { x, y };
		pair.second->location += (pair.second->target_location - pair.second->location) * (1.0f / 100.0f);
		radial += (360.0f / static_cast<float>(nodes.size())) * (3.14159f / 180.0f);
	}
}

void render_radial_dash_nodes(const glm::vec2 &ul, const glm::vec2 &lr) {
	for (auto &p : connections) {
		auto id = format("{}:{}", p->host, p->port);
		auto node_i = nodes.find(id);
		if (node_i == nodes.end()) {
			sf::core::logger->info("Spawned new node: {}", id);
			auto new_node = std::make_shared<node>();
			new_node->location = { 0, 0 };
			new_node->target_location = { 0, 0 };
			new_node->stream = p;
			nodes[id] = new_node;
		}
	}
	calculate_target_locations();
	const glm::vec2 center = { glm::round(ul.x + ((lr.x - ul.x) * 0.5f)), glm::round(ul.y + ((lr.y - ul.y) * 0.5f)) };
	const float radius = glm::round(glm::min(lr.x - ul.x, lr.y - ul.y) * 0.4f);
	for (auto &pair : nodes) {
		auto location = (pair.second->location * radius) + center;
		sf::core::canvas->emit(
			format("radial-dash-node-{}", pair.first),
			{ glm::vec2(glm::round(location.x), glm::round(location.y)) - 8.0f },
			{ glm::vec2(glm::round(location.x), glm::round(location.y)) + 8.0f },
			[](sf::ui::element *elm) {
				auto data = std::any_cast<node *>(elm->data);
				auto state = data->stream->state();
				if (data->last_net_state != state) {
					data->last_net_state = state;
					return sf::ui::redraw_deep;
				}
				return sf::ui::nothing;
			},
			[](NVGcontext *ctx, sf::ui::element *elm) {
				NVGcolor color;
				auto data = std::any_cast<node *>(elm->data);
				switch (data->last_net_state) {
					case sf::net::tcp_stream::connected:
						color = nvgRGB(0, 255, 0);
						break;
					case sf::net::tcp_stream::connecting:
						color = nvgRGB(255, 255, 0);
						break;
					case sf::net::tcp_stream::disconnected:
						color = nvgRGB(255, 0, 0);
						break;
					case sf::net::tcp_stream::dead:
						color = nvgRGB(255, 255, 255);
						break;
				}
				const glm::vec2 center = {
					glm::round(elm->ul.x + ((elm->lr.x - elm->ul.x) * 0.5f)),
					glm::round(elm->ul.y + ((elm->lr.y - elm->ul.y) * 0.5f))
				};
				const float radius = glm::round(glm::min(elm->lr.x - elm->ul.x, elm->lr.y - elm->ul.y) * 0.4f);
				nvgBeginPath(ctx);
				nvgCircle(ctx, center.x, center.y, radius);
				nvgStrokeColor(ctx, color);
				nvgStrokeWidth(ctx, 3.0f);
				nvgStroke(ctx);
			},
			pair.second.get()
		);
	}
}

void render_radial_dash(glm::vec2 ul, glm::vec2 lr) {
	sf::core::canvas->emit(
		"tab_a_content", ul, lr,
		[](sf::ui::element *elm) {
			return sf::ui::nothing;
		},
		[](NVGcontext *ctx, sf::ui::element *elm) {
			const glm::vec2 center = { glm::round(elm->ul.x + ((elm->lr.x - elm->ul.x) * 0.5f)), glm::round(elm->ul.y + ((elm->lr.y - elm->ul.y) * 0.5f)) };
			const float radius = glm::round(glm::min(elm->lr.x - elm->ul.x, elm->lr.y - elm->ul.y) * 0.4f);
			nvgBeginPath(ctx);
			nvgRect(ctx, elm->ul.x, elm->ul.y, elm->lr.x - elm->ul.x, elm->lr.y - elm->ul.y);
			nvgFillColor(ctx, nvgRGBA(16, 16, 16, 128));
			nvgFill(ctx);
			nvgBeginPath(ctx);
			nvgCircle(ctx, center.x, center.y, radius);
			nvgStrokeColor(ctx, nvgRGB(192, 192, 192));
			nvgStrokeWidth(ctx, 3.0f);
			nvgStroke(ctx);
		}
	);
	render_radial_dash_nodes(ul, lr);
}

void render_content() {
	emit_bg();
	glm::vec2 ul, lr;
	ul = { 0, 0 };
	lr = { sf::core::canvas->state->size.x, sf::core::canvas->state->size.y };
	render_radial_dash(ul, lr);
	emit_material_button("fancy-button", { 20, 20 }, { 220, 40 });
}

bool load_fonts() {
	if (!std::filesystem::exists("fonts")) {
		sandfox::core::logger->warn("Unable to find fonts folder.");
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

void on_new_image_data(std::filesystem::path path, std::shared_ptr<std::vector<char>> data) {
	if (path.string().rfind("icons/") == std::string::npos) return;
	for (int p = 0; p < data->size(); p += 4) {
		for (int i = 0; i < 3; i++) {
			(*data)[p + i] = 255 - (*data)[p + i];
		}
	}
}

int main() {
	sf::img::on_new_image_data = on_new_image_data;
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
		for (int i = 8000; i < 8020; i++) connections.push_back(std::make_shared<sf::net::tcp_stream>("127.0.0.1", i));
		while (sf::core::update());
		connections.clear();
		nodes.clear();
		sf::core::shutdown();
	}
	return 0;
}