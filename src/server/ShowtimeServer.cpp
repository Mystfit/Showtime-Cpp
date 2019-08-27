#include <ShowtimeServer.h>
#include "ZstLogging.h"
#include "ZstStage.h"

ShowtimeServer::ShowtimeServer(const std::string& name, int port) : m_server(std::make_shared<Showtime::detail::ZstStage>())
{
	m_server->init(name.c_str(), port);
}

void ShowtimeServer::destroy()
{
	m_server->destroy();
}
