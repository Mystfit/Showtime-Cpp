#pragma once

#include <fmt/format.h>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/expressions.hpp>

#include <ZstExports.h>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;

#define DEFAULT_LOG_FILE "showtime.log"
#define ZST_LOG_APP_CHANNEL "app"
#define ZST_LOG_NET_CHANNEL "net"
#define ZST_LOG_ENTITY_CHANNEL "entity"

// ----------------------------------------------------------------------------
// Logging interface

enum LogLevel
{
	debug = 0,
	notification,
	warn,
	error
};

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", LogLevel)
BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string)

typedef src::severity_channel_logger_mt<LogLevel, std::string> ZstLogger_mt;
BOOST_LOG_GLOBAL_LOGGER(ZstGlobalLogger, ZstLogger_mt)


namespace ZstLog {
	namespace internals {
		//switch (level) {
		//case LogLevel::debug: return 0x07;
		//case LogLevel::notification: return 0x0F;
		//case LogLevel::warn: return 0x0D;
		//case LogLevel::error: return 0x0E;
		//default: return 0x0F;

#ifdef WIN32
#define DEBUG_COLOUR (FOREGROUND_BLUE | FOREGROUND_GREEN)
#define NOTIF_COLOUR (FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN)
#define WARN_COLOUR (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define ERROR_COLOUR (FOREGROUND_RED | FOREGROUND_INTENSITY)
#define RESET 0x0F
#else
#define DEBUG_COLOUR \033[34m
#define NOTIF_COLOUR \033[1;37m
#define WARN_COLOUR \033[1;33m
#define ERROR_COLOUR \033[1;31m
#define RESET \033[0m
#endif

		ZST_EXPORT void entity_sink_message(LogLevel level, const char * msg);
		ZST_EXPORT void net_sink_message(LogLevel level, const char * msg);
		ZST_EXPORT void app_sink_message(LogLevel level, const char * msg);

		class coloured_console_sink : public boost::log::sinks::basic_formatted_sink_backend<char, boost::log::sinks::synchronized_feeding>
		{
		public:
			static void consume(boost::log::record_view const& rec, string_type const& formatted_string);
		};
	}

	ZST_EXPORT void init_logger(const char * logger_name);
	ZST_EXPORT void init_file_logging(const char * log_file_path = "");
	
	template <typename... Args>
	inline void net(LogLevel level, const char* msg, const Args&... vars)
	{
		internals::net_sink_message(level, fmt::format(msg, vars...).c_str());
	}

	inline void net(LogLevel level, const char* msg)
	{
		internals::net_sink_message(level, msg);
	}

	template <typename... Args>
	inline void entity(LogLevel level, const char* msg, const Args&... vars)
	{
		internals::entity_sink_message(level, fmt::format(msg, vars...).c_str());
	}

	inline void entity(LogLevel level, const char* msg)
	{
		internals::entity_sink_message(level, msg);
	}

	template <typename... Args>
	inline void app(LogLevel level, const char* msg, const Args&... vars)
	{
		internals::app_sink_message(level, fmt::format(msg, vars...).c_str());
	}

	inline void app(LogLevel level, const char* msg)
	{
		internals::app_sink_message(level, msg);
	}
};
