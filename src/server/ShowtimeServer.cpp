#include <ShowtimeServer.h>
#include "ZstLogging.h"
#include "ZstStage.h"

namespace showtime { 
	ShowtimeServer::ShowtimeServer(const std::string& name, int port) : m_server(std::make_shared<showtime::detail::ZstStage>())
	{
		m_server->init(name.c_str(), port);
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
