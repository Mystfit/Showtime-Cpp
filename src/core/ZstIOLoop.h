#pragma once

//Boost includes
#include <boost/asio.hpp>
#include <boost/thread.hpp>

struct ZstIOLoop {
public:
	ZstIOLoop() {};
	void operator()();
	boost::asio::io_context& IO_context();

private:
	boost::asio::io_context m_io;
};
