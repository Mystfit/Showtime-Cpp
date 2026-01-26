#include <showtime/ShowtimeServer.h>
#include "ZstStage.h"
#include <showtime/ZstLogging.h>

namespace showtime {
	ShowtimeServer::ShowtimeServer() : m_server(std::make_shared<showtime::detail::ZstStage>()){
	}

	ShowtimeServer::~ShowtimeServer()
	{
		// Explicitly destroy the server before the destructor completes
		// This ensures m_server is destroyed in a controlled manner while
		// ShowtimeServer is still fully valid
		if (m_server) {
			m_server->destroy();
			m_server.reset();
		}
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

	bool ShowtimeServer::save_session(const char* filepath)
	{
		return m_server->save_session(filepath);
	}

	bool ShowtimeServer::load_session(const char* filepath)
	{
		return m_server->load_session(filepath);
	}

	void ShowtimeServer::set_preserve_entities_on_disconnect(bool preserve)
	{
		m_server->set_preserve_entities_on_disconnect(preserve);
	}

	bool ShowtimeServer::get_preserve_entities_on_disconnect() const
	{
		return m_server->get_preserve_entities_on_disconnect();
	}

	void ShowtimeServer::set_auto_load_session(const char* filepath)
	{
		m_server->set_auto_load_session(filepath);
	}
}
