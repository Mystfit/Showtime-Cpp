#pragma once

//Boost includes
#include <boost/thread.hpp>
#include <boost/asio/io_context.hpp>
#include <showtime/ZstExports.h>
#include <showtime/ZstLogging.h>

namespace showtime {
class ZstLogAdaptor;
template<typename T> class ZstEventDispatcher;
}

struct ZstIOLoop {
public:
	ZST_EXPORT ZstIOLoop() {};
	ZST_EXPORT void operator()();
	ZST_EXPORT boost::asio::io_context& IO_context();

	// Set log context to be pushed when the IO loop thread starts
	ZST_EXPORT void set_log_context(std::weak_ptr<showtime::ZstEventDispatcher<showtime::ZstLogAdaptor>> ctx);

private:
	boost::asio::io_context m_io;
	std::weak_ptr<showtime::ZstEventDispatcher<showtime::ZstLogAdaptor>> m_log_context;
};
