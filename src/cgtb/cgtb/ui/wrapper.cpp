#include "wrapper.h"

#include "canvas.h"

cgtb::ui::wrapper::wrapper(const std::string_view &uuid) :
	uuid(uuid),
	poll(std::bind(&wrapper::poll_cb, this, std::placeholders::_1)),
	render(std::bind(&wrapper::render_cb, this, std::placeholders::_1, std::placeholders::_2)) {
				
}

int cgtb::ui::wrapper::emit(canvas *canvas) {
	return canvas->emit(uuid, body, poll, render);
}