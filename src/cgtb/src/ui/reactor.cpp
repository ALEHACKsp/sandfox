#include <cgtb/ui/reactor.h>
#include <cgtb/ui/element.h>

using namespace std::chrono;

cgtb::ui::reactor::reactor() {

	for (auto &e : click_attack) e = 1;
	for (auto &e : click_decay) e = 1;
	for (auto &e : punch) e = 0;
	for (auto &e : punch_attack) e = 1;
	for (auto &e : punch_decay) e = 1;
	for (auto &e : press) e = 0;
	for (auto &e : press_attack) e = 1;
	for (auto &e : press_decay) e = 1;
}

void cgtb::ui::reactor::update(const cgtb::ui::element &element) {

	auto now = high_resolution_clock::now();
	auto delta = last == time_point<high_resolution_clock>::max() ? 0.0 : static_cast<double>(duration_cast<milliseconds>(now - last).count()) * 0.001;

	last = now;

	if (element.hover) {
		hover = std::min(1.0, hover + (hover_attack * delta));
		contact = element.cursor;
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
			click[i].push_back({ element.cursor, click_attack[i] });
		} else punch[i] = std::max(0.0, punch[i] - (punch_decay[i] * delta));
		if (element.press[i]) press[i] = std::min(1.0, press[i] + (press_attack[i] * delta));
		else press[i] = std::max(0.0, press[i] - (press_decay[i] * delta));
	}
}