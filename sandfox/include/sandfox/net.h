#pragma once

#include <cstddef>
#include <vector>
#include <string>
#include <memory>

namespace sandfox::net {

	struct tcp_stream_data;

	class tcp_stream {

		tcp_stream_data *data;

	public:

		enum status {
			connected,
			connecting,
			disconnected,
			dead
		};

		const std::string host;
		const int port;

		tcp_stream(const std::string &host, const int &port);
		~tcp_stream();

		status state();
		std::shared_ptr<std::vector<std::byte>> recv();
	};
}