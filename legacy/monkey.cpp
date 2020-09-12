#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <uv.h>

#ifdef ENABLE_TW

#include <nlohmann/json.hpp>
#include <transwarp.h>

void run_tasks(transwarp::parallel &executor) {

	auto get_cfg = transwarp::make_task(transwarp::root, []() -> nlohmann::json {
		return {
			{ "customers", nlohmann::json::array() }
		};
	});

	auto process_cfg = transwarp::make_task(transwarp::consume, [](nlohmann::json cfg) -> nlohmann::json {
		try {
			spdlog::info("Adding new customer to registry...");
			nlohmann::json new_customer = {
				{ "name", "Brandon" },
				{ "age", "20089" }
			};
			cfg["customers"].push_back(new_customer);
			return cfg;
		} catch (...) {
			spdlog::error("The 'process_cfg' task encountered an exception.");
			return { };
		}
	}, get_cfg);

	auto push_cfg = transwarp::make_task(transwarp::consume, [](nlohmann::json cfg) {
		try {
			spdlog::info("Pushing new configurations...");
			spdlog::debug(cfg.dump());
			return true;
		} catch (...) {
			spdlog::error("The 'push_cfg' task encountered an exception.");
			return false;
		}
	}, process_cfg);

	push_cfg->schedule_all(executor);
	push_cfg->wait();
}

#endif

#include <chrono>
#include <memory>
#include <readerwriterqueue.h>

uv_loop_t *loop;

namespace ezuv {

	void on_alloc_buffer_cb(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
		if (auto base = reinterpret_cast<char *>(calloc(1, size)); base) {
			*buf = uv_buf_init(base, size);
			spdlog::debug("Allocated buffer for UV: handle:{}, size:{}, buf:{}", reinterpret_cast<void *>(handle), size, reinterpret_cast<void *>(buf));
		}
		else {
			*buf = uv_buf_init(0, 0);
			spdlog::error("Unable to allocate buffer for UV.");
		}
	}

	void on_close_free_cb(uv_handle_t *handle) {
		free(handle);
	}

	struct client_req_data {
		client_req_data() = delete;
		time_t start;
		char *query;
		size_t query_len;
		char *response;
		size_t response_len;
		int work_started;
		uv_tcp_t *client;
		uv_work_t *work_req;
		uv_write_t *write_req;
		// uv_timer_t *timer;
	};

	void free_client_req_data(client_req_data *data) {
		if (!data) return;
		if (data->client) uv_close(reinterpret_cast<uv_handle_t *>(data->client), on_close_free_cb);
		if (data->work_req) free(data->work_req);
		if (data->write_req) free(data->write_req);
		// if (data->timer) uv_close(reinterpret_cast<uv_handle_t *>(data->timer), on_close_free_cb);
		if (data->query) free(data->query);
		if (data->response) free(data->response);
		free(data);
		spdlog::debug("Freed UV client_req_data: {}", reinterpret_cast<void *>(data));
	}

	void on_write_end_cb(uv_write_t *req, int status) {
		free_client_req_data(reinterpret_cast<client_req_data *>(req->data));
		spdlog::debug("UV write completed: req:{}, status:{}", reinterpret_cast<void *>(req), status);
	}

	void on_post_process_command_cb(uv_work_t *req, int status) {
		auto data = reinterpret_cast<client_req_data *>(req->data);
		uv_buf_t buf = uv_buf_init(data->response, data->response_len);
		data->write_req = reinterpret_cast<uv_write_t *>(malloc(sizeof(*data->write_req)));
		data->write_req->data = data;
		// uv_timer_stop(data->timer);
		uv_write(data->write_req, reinterpret_cast<uv_stream_t *>(data->client), &buf, 1, on_write_end_cb);
		spdlog::debug("Completed UV post process command operations: req:{}, status:{}", reinterpret_cast<void *>(req), status);
	}

	void on_process_command_cb(uv_work_t *req) {
		auto data = reinterpret_cast<client_req_data *>(req->data);
		data->response = strdup("Work's done!");
		data->response_len = 13;
	}

	void on_read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
		auto client = reinterpret_cast<uv_tcp_t *>(stream);
		auto data = reinterpret_cast<client_req_data *>(stream->data);
		if (nread == -1 || nread == UV_EOF) {
			free(buf->base);
			// uv_timer_stop(data->timer);
			free_client_req_data(data);
			spdlog::debug("Completed UV stream read: stream:{}, nread:{}, buf:{}", reinterpret_cast<void *>(stream), nread, reinterpret_cast<const void *>(buf));
		} else {
			if (!data->query) {
				data->query = reinterpret_cast<char *>(malloc(nread));
				memcpy(data->query, buf->base, nread);
				data->query_len = nread;
				spdlog::debug("Prepped new UV client data buffer: stream:{}, nread:{}, buf:{}", reinterpret_cast<void *>(stream), nread, reinterpret_cast<const void *>(buf));
			} else {
				auto tmp = reinterpret_cast<char *>(realloc(data->query, data->query_len + nread));
				memcpy(tmp + data->query_len, buf->base, nread);
				data->query = tmp;
				data->query_len += nread;
				spdlog::debug("Resized UV client data buffer: stream:{}, nread:{}, buf:{}", reinterpret_cast<void *>(stream), nread, reinterpret_cast<const void *>(buf));
			}
			free(buf->base);
			if (!data->work_started && data->query_len) {
				data->work_req = reinterpret_cast<uv_work_t *>(malloc(sizeof(*data->work_req)));
				data->work_req->data = data;
				data->work_started = 1;
				uv_read_stop(stream);
				uv_queue_work(loop, data->work_req, on_process_command_cb, on_post_process_command_cb);
				spdlog::debug("Stopping UV stream reading: stream:{}, nread:{}, buf:{}", reinterpret_cast<void *>(stream), nread, reinterpret_cast<const void *>(buf));
			}
		}
	}

	/*
	void on_client_timeout_cb(uv_timer_t *handle) {
		auto data = reinterpret_cast<client_req_data *>(handle->data);
		uv_timer_stop(handle);
		if (data->work_started) return;
		free_client_req_data(data);
		spdlog::debug("UV client timed out: handle:{}", reinterpret_cast<void *>(handle));
	}
	*/

	void on_new_connection_cb(uv_stream_t *server, int status) {
		auto data = reinterpret_cast<client_req_data *>(calloc(1, sizeof(client_req_data)));
		data->start = time(0);
		auto client = reinterpret_cast<uv_tcp_t *>(malloc(sizeof(uv_tcp_t)));
		client->data = data;
		data->client = client;
		uv_tcp_init(loop, client);
		if (uv_accept(server, reinterpret_cast<uv_stream_t *>(client)) == 0) {
			auto timer = reinterpret_cast<uv_timer_t *>(malloc(sizeof(uv_timer_t)));
			timer->data = data;
			// data->timer = timer;
			uv_timer_init(loop, timer);
			uv_timer_set_repeat(timer, 1);
			// uv_timer_start(timer, on_client_timeout_cb, 10000, 20000);
			uv_read_start(reinterpret_cast<uv_stream_t *>(client), on_alloc_buffer_cb, on_read_cb);
			spdlog::debug("Accepted new UV connection: server:{}, status:{}", reinterpret_cast<void *>(server), status);
		} else {
			free_client_req_data(data);
			spdlog::error("Unable to accept new UV connection: server:{}, status:{}", reinterpret_cast<void *>(server), status);
		}
	}
}

#include <sstream>

void fancy(uv_getaddrinfo_t *req) {

	auto socket = reinterpret_cast<uv_tcp_t *>(malloc(sizeof(uv_tcp_t)));
	auto connect = reinterpret_cast<uv_connect_t *>(malloc(sizeof(uv_connect_t)));

	uv_tcp_init(loop, socket);

	socket->data = connect;

	spdlog::debug("init: socket:{}, connect:{}", reinterpret_cast<void *>(socket), reinterpret_cast<void *>(connect));

	uv_tcp_connect(connect, socket, req->addrinfo->ai_addr, [](uv_connect_t *req, int status) {

		spdlog::debug("connect: {}", status);

		uv_read_start(req->handle, [](uv_handle_t *handle, size_t size, uv_buf_t *buf) {

			if (auto base = reinterpret_cast<char *>(calloc(1, size)); base) *buf = uv_buf_init(base, size);
			else *buf = uv_buf_init(0, 0);

			spdlog::debug("new buf: base:{}, len:{}", reinterpret_cast<void *>(buf->base), buf->len);

		}, [](uv_stream_t *socket, ssize_t nread, const uv_buf_t *buf) {

			spdlog::debug("cb: socket:{}, connect:{}, buf:{}[{}], nread:{}", reinterpret_cast<void *>(socket), reinterpret_cast<void *>(socket->data), reinterpret_cast<void *>(buf->base), buf->len, nread);

			if (nread > 0) {
				std::stringstream ss;
				for (ssize_t i = 0; i < nread; i++) ss << buf->base[i];
				spdlog::info(ss.str());
			}

			free(buf->base);
			// free(socket->data);
			// free(socket);

		});
	});
}

int main() {

	auto logger = spdlog::stdout_color_mt("general");

	spdlog::set_default_logger(logger);
	spdlog::set_level(spdlog::level::debug);

	#ifdef ENABLE_TW
	transwarp::parallel executor { 4 };
	run_tasks(executor);
	#endif

	loop = uv_default_loop();

	/*

	uv_tcp_t server;
	struct sockaddr_in addr;

	spdlog::debug("uv_tcp_init: {}", uv_tcp_init(loop, &server));
	spdlog::debug("uv_ip4_addr: {}", uv_ip4_addr("0.0.0.0", 10000, &addr));
	spdlog::debug("uv_tcp_bind: {}", uv_tcp_bind(&server, reinterpret_cast<const struct sockaddr *>(&addr), 0));
	spdlog::debug("uv_listen: {}", uv_listen(reinterpret_cast<uv_stream_t *>(&server), 128, ezuv::on_new_connection_cb));

	spdlog::info("Listening for connections.");

	*/

	uv_getaddrinfo_t server_info;

	uv_getaddrinfo(
		loop,
		&server_info,
		[](uv_getaddrinfo_t *req, int status, addrinfo *res) {
			if (status == 0) fancy(req);
		},
		"10.0.0.2", "22", 0
	);

	return uv_run(loop, UV_RUN_DEFAULT);
}