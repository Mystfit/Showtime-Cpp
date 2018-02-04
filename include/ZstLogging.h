#pragma once

#include <spdlog/spdlog.h>

#define GLOBAL_CONSOLE "console"
#define PATTERN "[%H:%M:%S.%e] [PID:%P] [TID:%t] [%l] %v"
#define DEFAULT_LOG_FILE "showtime.log"

static std::shared_ptr<spdlog::logger> _logger;

extern "C" {

	static void zst_log_init(bool debug = false, bool log_to_file = false, const char * log_path = DEFAULT_LOG_FILE) {
		spdlog::set_async_mode(4096);

		try {
			//auto logger = spdlog::stdout_color_mt(GLOBAL_CONSOLE);
			if (log_to_file) {
				_logger = spdlog::basic_logger_mt(GLOBAL_CONSOLE, log_path);
			}
			else {
				_logger = spdlog::stdout_color_mt(GLOBAL_CONSOLE);
			}

			_logger->set_pattern(PATTERN);
			
			if(debug)
				_logger->set_level(spdlog::level::debug);
		}
		catch (spdlog::spdlog_ex e) {
			//Logger already exists, be quiet
		}
	}
}

//----------------------------------------------------------------------------
//Function forwards for spdlog so that we keep the logger reference in one dll

namespace ZstLog {
	template <typename Arg1, typename... Args>
	inline void trace(const char* fmt, const Arg1 &arg1, const Args&... args)
	{
		_logger->trace(fmt, arg1, args...);
	}

	template <typename Arg1, typename... Args>
	inline void debug(const char* fmt, const Arg1 &arg1, const Args&... args)
	{
		_logger->debug(fmt, arg1, args...);
	}

	template <typename Arg1, typename... Args>
	inline void info(const char* fmt, const Arg1 &arg1, const Args&... args)
	{
		_logger->info(fmt, arg1, args...);
	}

	template <typename Arg1, typename... Args>
	inline void warn(const char* fmt, const Arg1 &arg1, const Args&... args)
	{
		_logger->warn(fmt, arg1, args...);
	}

	template <typename Arg1, typename... Args>
	inline void error(const char* fmt, const Arg1 &arg1, const Args&... args)
	{
		_logger->error(fmt, arg1, args...);
	}

	template <typename Arg1, typename... Args>
	inline void critical(const char* fmt, const Arg1 &arg1, const Args&... args)
	{
		_logger->critical(fmt, arg1, args...);
	}

	template<typename T>
	inline void trace(const T& msg)
	{
		_logger->trace(msg);
	}

	template<typename T>
	inline void debug(const T& msg)
	{
		_logger->debug(msg);
	}

	template<typename T>
	inline void info(const T& msg)
	{
		_logger->info(msg);
	}

	template<typename T>
	inline void warn(const T& msg)
	{
		_logger->warn(msg);
	}

	template<typename T>
	inline void error(const T& msg)
	{
		_logger->error(msg);
	}

	template<typename T>
	inline void critical(const T& msg)
	{
		_logger->critical(msg);
	}
}

