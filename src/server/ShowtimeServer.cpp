#include <ShowtimeServer.h>
#include "ZstLogging.h"
#include "ZstStage.h"

namespace showtime { 
	ShowtimeServer::ShowtimeServer(const std::string& name, int port, bool unlisted) : m_server(std::make_shared<showtime::detail::ZstStage>())
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
