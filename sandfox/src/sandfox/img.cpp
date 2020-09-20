#include <sandfox/core.h>
#include <sandfox/img.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <map>
#include <glm/common.hpp>
#include <stb_image.h>
#include <stb_image_resize.h>
#include <svgdocument.h>
#include <nanovg.h>
#include <uv.h>

static unsigned int concatenate(unsigned x, unsigned y) {
	unsigned int pow = 10;
	while(y >= pow) pow *= 10;
	return x * pow + y;        
}

std::function<void(std::filesystem::path, std::shared_ptr<std::vector<char>>)> sandfox::img::on_new_image_data;

namespace sandfox::img {
	struct worker_baton {
		id key;
		std::shared_ptr<std::vector<char>> data;
		std::shared_ptr<spdlog::logger> logger;
	};
}

namespace sandfox::img {
	std::map<std::string, std::shared_ptr<std::vector<char>>> data_cache;
	std::map<std::string, std::map<unsigned int, std::shared_ptr<int>>> instance_cache;
}

void on_image_load(uv_work_t *request) {
	auto baton = reinterpret_cast<sandfox::img::worker_baton *>(request->data);
	auto fs_path = std::filesystem::path(std::get<0>(baton->key));
	if (!std::filesystem::exists(fs_path)) {
		baton->logger->error("File doesn't exist.");
		return;
	}
	auto extension = fs_path.extension().string();
	if (extension == ".svg") {
		lunasvg::SVGDocument doc;
		if (!doc.loadFromFile(fs_path.string())) {
			baton->logger->error("Unable to load SVG image.");
			return;
		}
		auto bm = doc.renderToBitmap(std::get<1>(baton->key).x, std::get<1>(baton->key).y);
		baton->data = std::make_shared<std::vector<char>>();
		baton->data->resize(std::get<1>(baton->key).x * std::get<1>(baton->key).y * 4);
		memcpy(baton->data->data(), bm.data(), baton->data->size());
	} else {
		int w, h;
		auto image_data = stbi_load(std::get<0>(baton->key).data(), &w, &h, 0, 4);
		if (!image_data) {
			baton->logger->error("Unable to load image.");
			return;
		}
		baton->data = std::make_shared<std::vector<char>>();
		if (w != std::get<1>(baton->key).x || h != std::get<1>(baton->key).y) {
			baton->logger->debug("Native size is {}x{}; resizing to {}x{}.", w, h, std::get<1>(baton->key).x, std::get<1>(baton->key).x);
			baton->data->resize(std::get<1>(baton->key).x * std::get<1>(baton->key).y * 4);
			if (stbir_resize_uint8(image_data, w, h, 0, reinterpret_cast<unsigned char *>(baton->data->data()), std::get<1>(baton->key).x, std::get<1>(baton->key).y, 0, 4)) {
				stbi_image_free(image_data);
				return;
			} else baton->logger->warn("Failed to resize. Keeping native size.");
		}
		baton->data->resize(w * h * 4);
		memcpy(baton->data->data(), image_data, baton->data->size());
		stbi_image_free(image_data);
	}
	if (sandfox::img::on_new_image_data) sandfox::img::on_new_image_data(fs_path, baton->data);
}

void on_image_load_done(uv_work_t *request, int status) {
	auto baton = reinterpret_cast<sandfox::img::worker_baton *>(request->data);
	if (baton->data) {
		if (int new_texture = nvgCreateImageRGBA(sandfox::core::nvg, std::get<1>(baton->key).x, std::get<1>(baton->key).y, 0, reinterpret_cast<const unsigned char *>(baton->data->data())); new_texture) {
			*sandfox::img::instance_cache[std::get<0>(baton->key)][concatenate(std::get<1>(baton->key).x, std::get<1>(baton->key).y)] = new_texture;
			sandfox::img::data_cache[std::get<0>(baton->key)] = baton->data;
			baton->logger->debug("Loaded into GL texture #{}.", new_texture);
		} else baton->logger->error("Unable to generate texture from image data.");
	} else baton->logger->warn("Unable to retrieve image data.");
	delete baton;
	delete request;
}

std::optional<std::shared_ptr<int>> sandfox::img::get(const id& key) {
	if (auto existing = find(key); existing.has_value()) return existing.value();
	auto new_cache_object = std::make_shared<int>();
	if (auto existing_data = data_cache.find(std::get<0>(key)); existing_data == data_cache.end()) {
		auto worker = new uv_work_t;
		auto baton = new worker_baton;
		worker->data = baton;
		baton->key = key;
		baton->logger = spdlog::stdout_color_mt(fmt::format("{} {}x{}", std::get<0>(key), std::get<1>(key).x, std::get<1>(key).y));
		baton->logger->set_level(sandfox::core::logger->level());
		baton->logger->set_pattern(core::default_logging_pattern);
		uv_queue_work(uv_default_loop(), worker, on_image_load, on_image_load_done);
		baton->logger->debug("Queued: {}", reinterpret_cast<void *>(worker));
		*new_cache_object = 0;
	} else {
		int new_img = nvgCreateImageRGBA(sandfox::core::nvg, std::get<1>(key).x, std::get<1>(key).y, 0, reinterpret_cast<const unsigned char *>(existing_data->second->data()));
		if (!new_img) return std::nullopt;
		*new_cache_object = new_img;
	}
	instance_cache[std::get<0>(key)][concatenate(std::get<1>(key).x, std::get<1>(key).y)] = new_cache_object;
	return new_cache_object;
}

std::optional<std::shared_ptr<int>> sandfox::img::find(const id& key) {
	if (auto key_location = instance_cache.find(std::get<0>(key)); key_location != instance_cache.end()) {
		for (auto &pair : key_location->second) {
			if (pair.first == concatenate(std::get<1>(key).x, std::get<1>(key).y)) {
				return pair.second;
			}
		}
	}
	return std::nullopt;
}

void sandfox::img::emit(ui::canvas *to, const std::string_view &uuid, const id &key, const glm::vec2 &center, const glm::vec2 &size) {
	glm::vec2 ul = { glm::round(center.x - size.x * 0.5f), glm::round(center.y - size.y * 0.5f) };
	glm::vec2 lr = { glm::round(ul.x + size.x), glm::round(ul.y + size.y) };
	to->emit(
		uuid,
		ul,
		lr,
		[](ui::element *elm) {
			return ui::nothing;
		}, [](NVGcontext *ctx, ui::element *elm) {
			auto img = img::get(std::any_cast<id>(elm->data));
			if (!img.has_value()) return;
			nvgBeginPath(ctx);
			float w = glm::round(elm->lr.x - elm->ul.x);
			float h = glm::round(elm->lr.y- elm->ul.y);
			nvgRect(ctx, elm->ul.x, elm->ul.y, w, h);
			nvgFillPaint(ctx, nvgImagePattern(ctx, elm->ul.x, elm->ul.y, w, h, 0, *img->get(), 1));
			nvgFill(ctx);
		},
		key
	);
}