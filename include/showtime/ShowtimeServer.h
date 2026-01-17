#pragma once

#include <memory>
#include <string>

#include "ZstConstants.h"
#include "ZstExports.h"

//Forwards
namespace showtime {
	namespace detail {
		class ZstStage;
	}


	class ZST_CLASS_EXPORTED ShowtimeServer
#ifndef SWIG
		: public std::enable_shared_from_this<ShowtimeServer>
#endif
	{
	public:
		ZST_SERVER_EXPORT ShowtimeServer();
		ZST_SERVER_EXPORT void init(const char* name = "stage", int port = -1, bool unlisted=false);

		//Disable copying
		ZST_SERVER_EXPORT ShowtimeServer(const ShowtimeServer& other) = delete;

		ZST_SERVER_EXPORT int port();
		ZST_SERVER_EXPORT void destroy();

		// Session management
		ZST_SERVER_EXPORT bool save_session(const char* filepath);
		ZST_SERVER_EXPORT bool load_session(const char* filepath);

		// Offline entity configuration
		ZST_SERVER_EXPORT void set_preserve_entities_on_disconnect(bool preserve);
		ZST_SERVER_EXPORT bool get_preserve_entities_on_disconnect() const;

		// Auto-load session on startup
		ZST_SERVER_EXPORT void set_auto_load_session(const char* filepath);

	private:

		std::shared_ptr<detail::ZstStage> m_server;
	};
}
