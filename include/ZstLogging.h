#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/fmt/fmt.h>
#include <Queue.h>
#include <string>
#include <sstream>
#include <spdlog/fmt/fmt.h>
#include <log4cplus/logger.h>

#define GLOBAL_CONSOLE "console"
#define PATTERN "[%H:%M:%S.%e] [PID:%P] [TID:%t] [%l] %v"
#define DEFAULT_LOG_FILE "showtime.log"



//----------------------------------------------------------------------------
//Function forwards for spdlog so that we keep the logger reference in one dll

class ZstExternalLog : public spdlog::sinks::sink
{
public:
	virtual ZST_EXPORT ~ZstExternalLog() {};

	virtual void log(const spdlog::details::log_msg& msg) override
	{
		// Your code here. 
		// details::log_msg is a struct containing the log entry info like level, timestamp, thread id etc.
		// msg.formatted contains the formatted log.
		// msg.raw contains pre formatted log
			
		//std::cout << msg.formatted.str();
		m_log_buffer.push(msg.formatted.str());
	}

	virtual void flush() override
	{
		//std::cout << std::flush;
		std::stringstream s;
		s << std::flush;
		m_log_buffer.push(s.str());
	}

	ZST_EXPORT void release_logs() {
		while (m_log_buffer.size()) {
			std::string msg = m_log_buffer.pop();
			log_to_external(msg.c_str());
		}
	}

	ZST_EXPORT virtual void log_to_external(const char * msg) {}
	
private:
	Queue<std::string> m_log_buffer;
};

namespace ZstLog {

	template <typename Arg1, typename... Args>
	inline void trace(const char* fmt, const Arg1 &arg1, const Args&... args)
	{
		if(_logger) _logger->trace(fmt, arg1, args...);
	}

	template <typename Arg1, typename... Args>
	inline void debug(const char* fmt, const Arg1 &arg1, const Args&... args)
	{
		if (_logger) _logger->debug(fmt, arg1, args...);
	}

	template <typename Arg1, typename... Args>
	inline void info(const char* fmt, const Arg1 &arg1, const Args&... args)
	{
		if (_logger) _logger->info(fmt, arg1, args...);
	}

	template <typename Arg1, typename... Args>
	inline void warn(const char* fmt, const Arg1 &arg1, const Args&... args)
	{
		if (_logger) _logger->warn(fmt, arg1, args...);
	}

	template <typename Arg1, typename... Args>
	inline void error(const char* fmt, const Arg1 &arg1, const Args&... args)
	{
		if (_logger) _logger->error(fmt, arg1, args...);
	}

	template <typename Arg1, typename... Args>
	inline void critical(const char* fmt, const Arg1 &arg1, const Args&... args)
	{
		if (_logger) _logger->critical(fmt, arg1, args...);
	}

	template<typename T>
	inline void trace(const T& msg)
	{
		if (_logger) _logger->trace(msg);
	}

	template<typename T>
	inline void debug(const T& msg)
	{
		if (_logger) _logger->debug(msg);
	}

	template<typename T>
	inline void info(const T& msg)
	{
		if (_logger) _logger->info(msg);
	}

	template<typename T>
	inline void warn(const T& msg)
	{
		if (_logger) _logger->warn(msg);
	}

	template<typename T>
	inline void error(const T& msg)
	{
		if (_logger) _logger->error(msg);
	}

	template<typename T>
	inline void critical(const T& msg)
	{
		if (_logger) _logger->critical(msg);
	}
}



static std::shared_ptr<spdlog::logger> _logger = NULL;
static ZstExternalLog * _ext_logger = NULL;

extern "C" 
{
	static void zst_log_init(bool debug = false, ZstExternalLog * external_logger = NULL) {
		spdlog::set_async_mode(4096);
		_ext_logger = external_logger;

		try {
			//auto logger = spdlog::stdout_color_mt(GLOBAL_CONSOLE);
			if (external_logger) {
				_logger = _logger = std::make_shared<spdlog::logger>(GLOBAL_CONSOLE, std::shared_ptr<ZstExternalLog>(external_logger));
			}
			else {
				_logger = spdlog::stdout_color_mt(GLOBAL_CONSOLE);
			}

			_logger->set_pattern(PATTERN);

			if (debug)
				_logger->set_level(spdlog::level::debug);
		}
		catch (spdlog::spdlog_ex e) {
			//Logger already exists, be quiet
		}
	}

	static void zst_log_destroy() {
		//_logger = NULL;
		_ext_logger = NULL;
	}
}