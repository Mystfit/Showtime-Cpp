#include <showtime/ShowtimeServer.h>
#include "ZstStage.h"
#include <showtime/ZstLogging.h>

namespace showtime { 
	ShowtimeServer::ShowtimeServer() : m_server(std::make_shared<showtime::detail::ZstStage>()){
	}

	void ShowtimeServer::init(const char* name, int port, bool unlisted)
	{
		m_server->init(name, port, unlisted);
	}

	int ShowtimeServer::port()
	{
		return m_server->port();
	}

	void ShowtimeServer::destroy()
	{
		m_server->destroy();
	}
}
