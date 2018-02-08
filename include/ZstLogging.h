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
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/win32consoleappender.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>

#define GLOBAL_CONSOLE "main"
#define PATTERN "[%H:%M:%S.%e] [PID:%P] [TID:%t] [%l] %v"
#define DEFAULT_LOG_FILE "showtime.log"

// ----------------------------------------------------------------------------
// Logging interface

namespace ZstLog {

	struct LoggerInfo {
		std::string name;
	};
	
	static LoggerInfo & main_logger(const char * name = "") {
		static LoggerInfo main = { std::string(name) };
		return main;
	}

	static void init_logger(const char * logger_name, bool debug = false) {
		log4cplus::initialize();
		//log4cplus::BasicConfigurator config;
		//config.configure();

		main_logger(logger_name);
		log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(main_logger().name));
		log4cplus::helpers::Properties props;
		props.setProperty(LOG4CPLUS_TEXT("AsyncAppend"), LOG4CPLUS_TEXT("true"));

#ifdef WIN32
		//log4cplus::SharedAppenderPtr append_to_win_console(new log4cplus::Win32ConsoleAppender(props));
		log4cplus::SharedAppenderPtr append_to_console(new log4cplus::ConsoleAppender(props));
#else
		log4cplus::SharedAppenderPtr append_to_console(new log4cplus::ConsoleAppender());
#endif
		std::string console_appender_name = fmt::format("{}_console", main_logger().name);
		append_to_console->setName(LOG4CPLUS_TEXT(console_appender_name));
		append_to_console->setLayout(std::unique_ptr<log4cplus::Layout>(new log4cplus::TTCCLayout()));
		logger.addAppender(append_to_console);
		
		if (debug) {
			logger.setLogLevel(log4cplus::DEBUG_LOG_LEVEL);
			log4cplus::helpers::LogLog::getLogLog()->setInternalDebugging(true);
		}
	}

	static void init_file_logging(const char * log_file_path = "") {
		log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(ZstLog::main_logger().name));
		std::string file_path(log_file_path);
		if (file_path.empty()) {
			file_path = fmt::format("{}.log", ZstLog::main_logger().name);
		}
		log4cplus::SharedFileAppenderPtr append_to_file(new log4cplus::RollingFileAppender(LOG4CPLUS_TEXT(file_path))); // 5 * 1024, 5, false, true)
		std::string file_appender_name = fmt::format("{}_file", ZstLog::main_logger().name);
		append_to_file->setName(LOG4CPLUS_TEXT(file_appender_name));
		append_to_file->setLayout(std::unique_ptr<log4cplus::Layout>(new log4cplus::TTCCLayout()));
		logger.addAppender(log4cplus::SharedAppenderPtr(append_to_file.get()));
	}

	static void destroy_logger() {
		log4cplus::Logger::shutdown();
	}

	template <typename... Args>
	static void trace(const char* fmt, const Args&... args)
	{
		std::string msg = fmt::format(fmt, args...);
        log4cplus::Logger::getInstance(ZstLog::main_logger().name).log(log4cplus::TRACE_LOG_LEVEL, msg);
	}

	template <typename... Args>
	static void debug(const char* fmt, const Args&... args)
	{
		std::string msg = fmt::format(fmt, args...);
        log4cplus::Logger::getInstance(ZstLog::main_logger().name).log(log4cplus::DEBUG_LOG_LEVEL, msg);
	}

	template <typename... Args>
	static void info(const char* fmt, const Args&... args)
	{
		std::string msg = fmt::format(fmt, args...);
        log4cplus::Logger::getInstance(ZstLog::main_logger().name).log(log4cplus::INFO_LOG_LEVEL, msg);
	}

	template <typename... Args>
	static void warn(const char* fmt, const Args&... args)
	{
		std::string msg = fmt::format(fmt, args...);
        log4cplus::Logger::getInstance(ZstLog::main_logger().name).log(log4cplus::WARN_LOG_LEVEL, msg);
	}

	template <typename... Args>
	static void error(const char* fmt, const Args&... args)
	{
		std::string msg = fmt::format(fmt, args...);
        log4cplus::Logger::getInstance(ZstLog::main_logger().name).log(log4cplus::ERROR_LOG_LEVEL, msg);
	}

	static void trace(const char * msg)
	{
        log4cplus::Logger::getInstance(ZstLog::main_logger().name).log(log4cplus::TRACE_LOG_LEVEL, msg);
	}

	static void debug(const char * msg)
	{
        log4cplus::Logger::getInstance(ZstLog::main_logger().name).log(log4cplus::DEBUG_LOG_LEVEL, msg);
	}

	static void info(const char * msg)
	{
        log4cplus::Logger::getInstance(ZstLog::main_logger().name).log(log4cplus::INFO_LOG_LEVEL, msg);
	}

	static void warn(const char * msg)
	{
        log4cplus::Logger::getInstance(ZstLog::main_logger().name).log(log4cplus::WARN_LOG_LEVEL, msg);
	}

	static void error(const char * msg)
	{
        log4cplus::Logger::getInstance(ZstLog::main_logger().name).log(log4cplus::ERROR_LOG_LEVEL, msg);
	}
}
