#include "ZstIOLoop.h"
#include <boost/asio/executor_work_guard.hpp>

void ZstIOLoop::operator()()
{
	boost::this_thread::interruption_point();

	// Push log context for this thread if set
	showtime::Log::ScopedContext log_ctx(m_log_context);

	//Give the event loop some work to do so it doesn't insta-quit
	auto work = boost::asio::make_work_guard(m_io);

	//Run the event loop (blocks this thread)
	m_io.run();
}

boost::asio::io_context& ZstIOLoop::IO_context()
{
	return m_io;
}

void ZstIOLoop::set_log_context(std::weak_ptr<showtime::ZstEventDispatcher<showtime::ZstLogAdaptor>> ctx)
{
	m_log_context = ctx;
}
