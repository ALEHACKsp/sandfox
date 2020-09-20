#include <sandfox/ui.h>
#include <spdlog/spdlog.h>
#include <nanovg.h>

static inline bool contains(const std::pair<glm::vec2, glm::vec2> &a, const glm::vec2 &p) {
	return p.x >= a.first.x && p.x < a.second.x && p.y >= a.first.y && p.y < a.second.y;
}

static inline bool intersecting(const std::pair<glm::vec2, glm::vec2> &a, const std::pair<glm::vec2, glm::vec2> &b) {
	return a.first.x < b.second.x && a.second.x > b.first.x && a.first.y < b.second.y && a.second.y > b.first.y;
}

sandfox::ui::reactor::reactor() {
	for (auto &e : click_attack) e = 1;
	for (auto &e : click_decay) e = 1;
	for (auto &e : punch) e = 0;
	for (auto &e : punch_attack) e = 1;
	for (auto &e : punch_decay) e = 1;
	for (auto &e : press) e = 0;
	for (auto &e : press_attack) e = 1;
	for (auto &e : press_decay) e = 1;
}

void sandfox::ui::reactor::update(const sandfox::ui::element &element) {
	auto now = std::chrono::high_resolution_clock::now();
	auto delta = last == std::chrono::time_point<std::chrono::high_resolution_clock>::max() ? 0.0 : static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count()) * 0.001;
	last = now;
	if (element.hover) {
		hover = std::min(1.0, hover + (hover_attack * delta));
		contact = element.cur;
	} else hover = std::max(0.0, hover - (hover_decay * delta));
	for (int i = 0; i < 8; i++) {
		for (int c = 0; c < click[i].size(); c++) {
			click[i][c].second -= click_decay[i] * delta;
			if (click[i][c].second <= 0) {
				click[i].erase(click[i].begin() + c);
				c--;
			}
		}
	}
	for (int i = 0; i < 8; i++) {
		if (element.click[i]) {
			punch[i] = std::min(1.0, punch[i] + punch_attack[i]);
			click[i].push_back({ element.cur, click_attack[i] });
		} else punch[i] = std::max(0.0, punch[i] - (punch_decay[i] * delta));
		if (element.press[i]) press[i] = std::min(1.0, press[i] + (press_attack[i] * delta));
		else press[i] = std::max(0.0, press[i] - (press_decay[i] * delta));
	}
}

void sandfox::ui::canvas::begin() {
	proposed.resize(1);
	proposed[0].clear();
}

bool sandfox::ui::canvas::end() {
	bool need_swap = false;
	for (auto &absent_element : absent) dirty.push_back({ 0, { absent_element.second.second.ul, absent_element.second.second.lr } });
	absent.clear();
	for (int layer = 0; layer < proposed.size(); layer++) {
		for (auto &E : proposed[layer]) {
			if (state->cursor_enabled) E.second.cur = { state->cur.x - E.second.ul.x, state->cur.y - E.second.ul.y };
			E.second.hover = state->cursor_enabled && contains({ E.second.ul, E.second.lr }, state->cur);
			for (int i = 0; i < 8; i++) {
				E.second.click[i] = E.second.hover && state->mouse_buttons[i] && !state->mouse_buttons_previous[i];
				E.second.press[i] = E.second.hover && state->mouse_buttons[i] && state->mouse_buttons_previous[i];
			}
			auto response = E.second.poll(&E.second);
			if (response == render_action::redraw) dirty.push_back({ layer, { E.second.ul, E.second.lr } });
			else if (response == render_action::redraw_deep) dirty.push_back({ 0, { E.second.ul, E.second.lr } });
		}
	}
	if (dirty.size()) {
		int render_num = 0;
		nvgBeginFrame(state->nvgc, state->size.x, state->size.y, 1);
		for (int layer = 0; layer < proposed.size(); layer++) {
			for (auto &element : proposed[layer]) {
				nvgScissor(state->nvgc, element.second.ul.x, element.second.ul.y, element.second.lr.x - element.second.ul.x, element.second.lr.y - element.second.ul.y);
				nvgSave(state->nvgc);
				element.second.render(state->nvgc, &element.second);
				nvgRestore(state->nvgc);
				nvgResetScissor(state->nvgc);
				need_swap = true;
				render_num++;
			}
		}
		nvgEndFrame(state->nvgc);
		/*
		nvgBeginFrame(state->nvgc, state->size.x, state->size.y, 1);
		for (auto &dirty_area : dirty) {
			for (int layer = dirty_area.first; layer < proposed.size(); layer++) {
				for (auto &E : proposed[layer]) {
					if (!intersecting({ E.second.ul, E.second.lr }, dirty_area.second)) continue;
					nvgScissor(state->nvgc, dirty_area.second.first.x, dirty_area.second.first.y, dirty_area.second.second.x - dirty_area.second.first.x, dirty_area.second.second.y - dirty_area.second.first.y);
					nvgIntersectScissor(state->nvgc, E.second.ul.x, E.second.ul.y, E.second.lr.x - E.second.ul.x, E.second.lr.y - E.second.ul.y);
					nvgSave(state->nvgc);
					E.second.render(state->nvgc, &E.second);
					nvgRestore(state->nvgc);
					nvgResetScissor(state->nvgc);
					need_swap = true;
					render_num++;
				}
			}
		}
		nvgEndFrame(state->nvgc);
		*/
		spdlog::info("Updated: {} dirty areas, {} render calls", dirty.size(), render_num);
		dirty.clear();
	}
	finalized = proposed;
	for (int finalized_layer_index = 0; finalized_layer_index < finalized.size(); finalized_layer_index++)
		for (auto &finalized_element : finalized[finalized_layer_index])
			absent[finalized_element.first] = { finalized_layer_index, finalized_element.second };
	state->mouse_buttons_previous = state->mouse_buttons;
	return need_swap;
}

int sandfox::ui::canvas::emit(const std::string_view &uuid, const glm::vec2 &ul, const glm::vec2 &lr, std::function<render_action(element *)> poll, std::function<void(NVGcontext *, element *)> render, std::any data) {
	absent.erase(uuid.data());
	element tmp;
	int to_layer = -1, prev_layer = -1;
	for (int layer = 0; layer < finalized.size(); layer++) {
		if (finalized[layer].find(uuid.data()) == finalized[layer].end()) continue;
		prev_layer = layer;
		break;
	}
	if (prev_layer >= 0) tmp = finalized[prev_layer][uuid.data()];
	tmp.ul = ul;
	tmp.lr = lr;
	tmp.poll = poll;
	tmp.render = render;
	if (data.has_value()) tmp.data = data;
	for (int layer = 0; layer < proposed.size(); layer++) {
		bool collision = false;
		for (auto &other : proposed[layer]) {
			if (intersecting({ other.second.ul, other.second.lr }, { ul, lr })) {
				collision = true;
				break;
			}
		}
		if (!collision) {
			to_layer = layer;
			break;
		}
	}
	if (to_layer == -1) {
		proposed.push_back({});
		to_layer = proposed.size() - 1;
	}
	if (prev_layer == -1) dirty.push_back({ 0, { ul, lr } });
	else {
		auto &prev_inst = finalized[prev_layer][uuid.data()];
		tmp.prev = &prev_inst;
		if (prev_inst.ul != ul || prev_inst.lr != lr) {
			dirty.push_back({ 0, { ul, lr } });
			dirty.push_back({ 0, { prev_inst.ul, prev_inst.lr } });
		} else if (prev_layer != to_layer) dirty.push_back({ std::min(std::min(prev_layer, to_layer), 0), { ul, lr } });
	}
	proposed[to_layer][uuid.data()] = tmp;
	return to_layer;
}