#pragma once

//Boost includes
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "ZstExports.h"

struct ZstIOLoop {
public:
	ZST_EXPORT ZstIOLoop() {};
	ZST_EXPORT void operator()();
	ZST_EXPORT boost::asio::io_context& IO_context();

private:
	boost::asio::io_context m_io;
};
