#include <sandfox/core.h>
#include <sandfox/net.h>

#include <uv.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <mutex>

namespace sandfox::net {

	struct tcp_stream_data {
	
		std::mutex mutex;
		uv_tcp_t socket;
		uv_connect_t connection;
		uv_timer_t timer;
		struct sockaddr_in address;
		std::shared_ptr<spdlog::logger> logger;
		int wait_ms;
		std::atomic_bool persist;
		std::atomic_char state;
		std::shared_ptr<std::vector<std::byte>> recv_buffer;
	
		void reinitialize_socket();
		void begin_connect();
		void begin_connect_later();

		~tcp_stream_data() {
			uv_close(reinterpret_cast<uv_handle_t *>(&timer), [](uv_handle_t *handle) { });
		}
	};
}

static void on_close_socket(uv_handle_t *handle) {
	auto self = reinterpret_cast<sandfox::net::tcp_stream_data *>(handle->data);
	if (self->persist) {
		if (uv_tcp_init(self->socket.loop, &self->socket) != 0) {
			self->logger->warn("Unable to reinitialize socket structure.");
			return;
		}
		self->begin_connect_later();
	} else {
		self->state = (char)sandfox::net::tcp_stream::status::dead;
		self->mutex.lock();
		self->logger->debug("Abandoned. Queueing destruction.");
		auto req = new uv_work_t;
		req->data = self;
		uv_queue_work(self->socket.loop, req, [](uv_work_t *req) { /* no op */ }, [](uv_work_t *req, int status) {
			delete reinterpret_cast<sandfox::net::tcp_stream_data *>(req->data);
			delete req;
		});
		self->mutex.unlock();
	}
}

static void allocate_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
	*buf = uv_buf_init((char*) malloc(suggested_size), suggested_size);
}

static void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *data) {
	auto self = reinterpret_cast<sandfox::net::tcp_stream_data *>(stream->data);
	if (nread == UV_EOF || nread == 0) self->logger->warn("End of stream indicated.");
	else if (nread > 0) {
		self->mutex.lock();
		if (!self->recv_buffer) self->recv_buffer = std::make_shared<std::vector<std::byte>>();
		self->recv_buffer->resize(self->recv_buffer->size() + nread);
		memcpy(&((*self->recv_buffer)[self->recv_buffer->size() - nread]), data->base, nread);
		self->mutex.unlock();
		return;
	} else self->logger->warn("Unable to read the stream: {}", uv_strerror(nread));
	self->state = (char)sandfox::net::tcp_stream::status::disconnected;
	self->reinitialize_socket();
}

static void on_connect(uv_connect_t *connection, int status) {
	auto self = reinterpret_cast<sandfox::net::tcp_stream_data *>(connection->data);
	if (status == 0) {
		self->state = (char)sandfox::net::tcp_stream::status::connected;
		uv_timer_stop(&self->timer);
		self->wait_ms = 10000;
		self->logger->debug("Connected");
		if (uv_read_start(connection->handle, allocate_buffer, on_read) != 0) self->logger->warn("Unable to start reading the stream.");
		else return;
	} else self->logger->warn("Unable to connect: {}", uv_strerror(status));
	self->wait_ms = 60000;
	if (self->persist) {
		self->state = (char)sandfox::net::tcp_stream::status::disconnected;
		self->begin_connect_later();
	} else self->state = (char)sandfox::net::tcp_stream::status::dead;
}

sandfox::net::tcp_stream::tcp_stream(const std::string &host, const int &port) : host(host), port(port) {
	data = new tcp_stream_data;
	data->logger = spdlog::stdout_color_mt(fmt::format("TCP {} {}:{}", reinterpret_cast<void *>(this), host, port));
	data->logger->set_level(spdlog::level::debug);
	data->logger->set_pattern(sandfox::core::default_logging_pattern);
	if (uv_tcp_init(uv_default_loop(), &data->socket) != 0) data->logger->warn("Unable to initialize socket structure.");
	if (uv_timer_init(uv_default_loop(), &data->timer) != 0) data->logger->warn("Unable to initialize timer structure.");
	if (uv_ip4_addr(host.data(), port, &data->address) != 0) data->logger->warn("Unable to populate address structure.");
	data->logger->debug("Socket Handle @ {}, Timer Handle @ {}", reinterpret_cast<void *>(&data->socket), reinterpret_cast<void *>(&data->timer));
	data->socket.data = data;
	data->connection.data = data;
	data->timer.data = data;
	data->wait_ms = 10000;
	data->persist = true;
	data->begin_connect();
}

sandfox::net::tcp_stream::~tcp_stream() {
	data->mutex.lock();
	data->persist = false;
	data->reinitialize_socket();
	data->mutex.unlock();
}

void sandfox::net::tcp_stream_data::reinitialize_socket() {
	uv_timer_stop(&timer);
	uv_read_stop(reinterpret_cast<uv_stream_t *>(&socket));
	uv_close(reinterpret_cast<uv_handle_t *>(&socket), on_close_socket);
} 

void sandfox::net::tcp_stream_data::begin_connect() {
	int r = uv_tcp_connect(&connection, &socket, reinterpret_cast<const struct sockaddr *>(&address), on_connect);
	if (r == 0) {
		state = (char)sandfox::net::tcp_stream::status::connecting;
		return;
	}
	logger->warn("Unable to queue connection attempt: {}", uv_strerror(r));
	uv_timer_stop(&timer);
	state = (char)sandfox::net::tcp_stream::status::dead;
}

void sandfox::net::tcp_stream_data::begin_connect_later() {
	if (uv_timer_start(&timer, [](uv_timer_t *timer) {
		auto self = reinterpret_cast<tcp_stream_data *>(timer->data);
		self->begin_connect();
	}, wait_ms, wait_ms) != 0) {
		logger->warn("Unable to start reconnection timer.");
		state = (char)sandfox::net::tcp_stream::status::dead;
	}
	else logger->debug("Retrying later.");
}

sandfox::net::tcp_stream::status sandfox::net::tcp_stream::state() {
	char cp = data->state;
	return (status)(int)(cp);
}

std::shared_ptr<std::vector<std::byte>> sandfox::net::tcp_stream::recv() {
	std::lock_guard guard(data->mutex);
	auto ptr = data->recv_buffer;
	data->recv_buffer = { };
	return ptr;
}