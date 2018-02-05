#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/fmt/fmt.h>
#include <Queue.h>
#include <string>
#include <sstream>

#include <spdlog/fmt/fmt.h>
#include <log4cplus/logger.h>
#include <log4cplus/loglevel.h>
#include <log4cplus/layout.h>
#include <log4cplus/helpers/snprintf.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/initializer.h>

#define GLOBAL_CONSOLE "main"
#define PATTERN "[%H:%M:%S.%e] [PID:%P] [TID:%t] [%l] %v"
#define DEFAULT_LOG_FILE "showtime.log"

//----------------------------------------------------------------------------
//Function forwards for spdlog so that we keep the logger reference in one dll

class ZstExternalLog
{
public:
	virtual ZST_EXPORT ~ZstExternalLog() {};

	virtual void log(const char * msg)
	{
		m_log_buffer.push(msg);
	}

	virtual void flush()
	{
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

	template <typename... Args>
	static void trace(const char* fmt, const Args&... args)
	{
		std::string msg = fmt::format(fmt, args...);
		LOG4CPLUS_TRACE(log4cplus::Logger::getRoot(), "\x1b[36m" << msg << "\x1b[0m");
	}

	template <typename... Args>
	static void debug(const char* fmt, const Args&... args)
	{
		std::string msg = fmt::format(fmt, args...);
		LOG4CPLUS_DEBUG(log4cplus::Logger::getRoot(), "\x1b[32m" << msg << "\x1b[0m");
	}

	template <typename... Args>
	static void info(const char* fmt, const Args&... args)
	{
		std::string msg = fmt::format(fmt, args...);
		LOG4CPLUS_INFO(log4cplus::Logger::getRoot(), msg);
	}

	template <typename... Args>
	static void warn(const char* fmt, const Args&... args)
	{
		std::string msg = fmt::format(fmt, args...);
		LOG4CPLUS_WARN(log4cplus::Logger::getRoot(), "\x1b[33m" << msg << "\x1b[0m");
	}

	template <typename... Args>
	static void error(const char* fmt, const Args&... args)
	{
		std::string msg = fmt::format(fmt, args...);
		LOG4CPLUS_ERROR(log4cplus::Logger::getRoot(), "\x1b[31m" << msg << "\x1b[0m");
	}

	static void trace(const char * msg)
	{
		LOG4CPLUS_TRACE(log4cplus::Logger::getRoot(), "\x1b[36m" << msg << "\x1b[0m");
	}

	static void debug(const char * msg)
	{
		LOG4CPLUS_DEBUG(log4cplus::Logger::getRoot(), "\x1b[32m" << msg << "\x1b[0m");
	}

	static void info(const char * msg)
	{
		LOG4CPLUS_INFO(log4cplus::Logger::getRoot(), msg);
	}

	static void warn(const char * msg)
	{
		LOG4CPLUS_WARN(log4cplus::Logger::getRoot(), "\x1b[33m" << msg << "\x1b[0m");
	}

	static void error(const char * msg)
	{
		LOG4CPLUS_ERROR(log4cplus::Logger::getRoot(), "\x1b[31m" << msg << "\x1b[0m");
	}
}



static ZstExternalLog * _ext_logger = NULL;

extern "C" 
{
	static void zst_log_init(bool debug = false, ZstExternalLog * external_logger = NULL) {
		log4cplus::initialize();
		log4cplus::BasicConfigurator config;
		config.configure();

		_ext_logger = external_logger;		
		if (external_logger) {

		}
		else {
			log4cplus::Logger logger = log4cplus::Logger::getRoot();
			/*log4cplus::SharedAppenderPtr append_1(new log4cplus::ConsoleAppender());
			append_1->setName(LOG4CPLUS_TEXT(GLOBAL_CONSOLE));
			append_1->setLayout(std::unique_ptr<log4cplus::Layout>(new log4cplus::TTCCLayout()));
			logger.addAppender(append_1);*/

			if (debug)
				logger.setLogLevel(log4cplus::DEBUG_LOG_LEVEL);
		}
	}

	static void zst_log_destroy() {
		//_logger = NULL;
		_ext_logger = NULL;
	}
}