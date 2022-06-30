#include "ZstIOLoop.h"

void ZstIOLoop::operator()()
{
	boost::this_thread::interruption_point();

	//Give the event loop some work to do so it doesn't insta-quit
	boost::asio::io_context::work work(m_io);

	//Run the event loop (blocks this thread)
	m_io.run();
}

boost::asio::io_context& ZstIOLoop::IO_context()
{
	return m_io;
}
