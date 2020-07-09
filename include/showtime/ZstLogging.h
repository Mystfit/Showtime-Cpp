#pragma once
#include <streambuf>
#include <ostream>

#ifndef FMT_HEADER_ONLY
	#define FMT_HEADER_ONLY
	#if defined(WIN32) && !defined(WIN32_LEAN_AND_MEAN)
		#define WIN32_LEAN_AND_MEAN
		#include <fmt/format.h>
		#undef WIN32_LEAN_AND_MEAN
	#else
		#include <fmt/format.h>
	#endif
	#undef FMT_HEADER_ONLY
#else //FMT_HEADER_ONLY
	#include <fmt/format.h>
#endif

#include <showtime/ZstExports.h>

#define DEFAULT_LOG_FILE "showtime.log"
#define ZST_LOG_APP_CHANNEL "app"
#define ZST_LOG_NET_CHANNEL "net"
#define ZST_LOG_SERVER_CHANNEL "srv"
#define ZST_LOG_ENTITY_CHANNEL "ent"

// ----------------------------------------------------------------------------
// Logging interface


namespace showtime {

	// Forwards
	class ZstLogAdaptor;

	template<typename T>
	class ZstEventDispatcher;

	namespace Log {
		enum Level
		{
			debug = 0,
			notification,
			warn,
			error,
			Levelsize
		};

		struct Record {
			unsigned int line_ID;
			std::string process_name;
			std::string threadID;
			Level level;
			std::string channel;
			std::string message;
		};

		namespace internals {
			//switch (level) {
			//case Log::Level::debug: return 0x07;
			//case Log::Level::notification: return 0x0F;
			//case Log::Level::warn: return 0x0D;
			//case Log::Level::error: return 0x0E;
			//default: return 0x0F;

#ifdef WIN32
#define DEBUG_COLOUR (FOREGROUND_BLUE | FOREGROUND_GREEN)
#define NOTIF_COLOUR (FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN)
#define WARN_COLOUR (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define ERROR_COLOUR (FOREGROUND_RED | FOREGROUND_INTENSITY)
#define RESET_COLOUR 0x0F
#else
#define DEBUG_COLOUR \033[34m
#define NOTIF_COLOUR \033[1;37m
#define WARN_COLOUR \033[1;33m
#define ERROR_COLOUR \033[1;31m
#define RESET_COLOUR \033[0m
#endif

			ZST_EXPORT void entity_sink_message(Level level, const char* msg);
			ZST_EXPORT void net_sink_message(Level level, const char* msg);
			ZST_EXPORT void server_sink_message(Level level, const char* msg);
			ZST_EXPORT void app_sink_message(Level level, const char* msg);
			static bool _logging = false;
		}		

		ZST_EXPORT void init_logger(const char* logger_name, Level level, std::shared_ptr<ZstEventDispatcher<ZstLogAdaptor> >& log_events);
		ZST_EXPORT void init_file_logging(const char* log_file_path = "");
		ZST_EXPORT const char* get_severity_str(Level level);

		template <typename... Args>
		inline void net(Level level, const char* msg, const Args&... vars)
		{
			internals::net_sink_message(level, fmt::format(msg, vars...).c_str());
		}

		inline void net(Level level, const char* msg)
		{
			internals::net_sink_message(level, msg);
		}

		template <typename... Args>
		inline void server(Level level, const char* msg, const Args&... vars)
		{
			internals::server_sink_message(level, fmt::format(msg, vars...).c_str());
		}

		inline void server(Level level, const char* msg)
		{
			internals::server_sink_message(level, msg);
		}

		template <typename... Args>
		inline void entity(Level level, const char* msg, const Args&... vars)
		{
			internals::entity_sink_message(level, fmt::format(msg, vars...).c_str());
		}

		inline void entity(Level level, const char* msg)
		{
			internals::entity_sink_message(level, msg);
		}

		template <typename... Args>
		inline void app(Level level, const char* msg, const Args&... vars)
		{
			internals::app_sink_message(level, fmt::format(msg, vars...).c_str());
		}

		inline void app(Level level, const char* msg)
		{
			internals::app_sink_message(level, msg);
		}
	}
};
