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
		ZST_SERVER_EXPORT ShowtimeServer(const std::string & name, int port = -1);

		ZST_SERVER_EXPORT int port();

		//Disable copying
		ZST_SERVER_EXPORT ShowtimeServer(const ShowtimeServer& other) = delete;

		ZST_SERVER_EXPORT void destroy();

	private:
		std::shared_ptr<detail::ZstStage> m_server;
	};
}
