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


	class ZST_CLASS_EXPORTED ShowtimeServer : private std::enable_shared_from_this<ShowtimeServer>
	{
	public:
		ZST_SERVER_EXPORT ShowtimeServer();
		ZST_SERVER_EXPORT void init(const std::string & name="stage", int port = -1, bool unlisted=false);
		
		//Disable copying
		ZST_SERVER_EXPORT ShowtimeServer(const ShowtimeServer& other) = delete;
		
		ZST_SERVER_EXPORT int port();
		ZST_SERVER_EXPORT void destroy();

	private:
		
		std::shared_ptr<detail::ZstStage> m_server;
	};
}
