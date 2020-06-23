#include <showtime/ShowtimeServer.h>
#include <showtime/ZstLogging.h>
#include "ZstStage.h"

namespace showtime { 
	ShowtimeServer::ShowtimeServer() : m_server(std::make_shared<showtime::detail::ZstStage>()){
	}

	void ShowtimeServer::init(const std::string & name, int port, bool unlisted)
	{
		m_server->init(port);
		if (!unlisted) {
			m_server->start_broadcasting(name.c_str());
		}
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
