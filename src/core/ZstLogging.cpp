#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/attributes/current_process_name.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/expressions.hpp>
#include <boost/core/null_deleter.hpp>
#include <fstream>
#ifdef WIN32
#include <windows.h>
#endif
#include <ZstLogging.h>

// Boost namespaces 
namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;

// Log attributes
BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", LogLevel)
BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(process_name, "ProcessName", std::string)

// Global logger
typedef src::severity_channel_logger_mt<LogLevel, std::string> ZstLogger_mt;
BOOST_LOG_GLOBAL_LOGGER(ZstGlobalLogger, ZstLogger_mt)

// Global logger init
BOOST_LOG_GLOBAL_LOGGER_INIT(ZstGlobalLogger, ZstLogger_mt) {
	ZstLogger_mt logger = ZstLogger_mt();
	return logger;
}

// Internal namespace extension
namespace ZstLog {
	namespace internals {
		// Custom Windows sink
		class coloured_console_sink : public boost::log::sinks::basic_formatted_sink_backend<char, boost::log::sinks::synchronized_feeding>
		{
		public:
			static void consume(boost::log::record_view const& rec, string_type const& formatted_string);
		};
	}
}
using namespace ZstLog;


void ZstLog::init_logger(const char * logger_name, LogLevel level)
{
#ifdef WIN32
	typedef boost::log::sinks::synchronous_sink<ZstLog::internals::coloured_console_sink> coloured_console_sink_t;
	auto sink = boost::make_shared<coloured_console_sink_t>();
#else
	typedef sinks::synchronous_sink<sinks::text_ostream_backend> text_sink;
	boost::shared_ptr<text_sink> sink = boost::make_shared< text_sink >();
	boost::shared_ptr<std::ostream> stream(&std::clog, boost::null_deleter());
	sink->locked_backend()->add_stream(stream);
#endif
	typedef expr::channel_severity_filter_actor< std::string, LogLevel > min_severity_filter;
	min_severity_filter min_severity = expr::channel_severity_filter(channel, severity);
	// Set up the minimum severity levels for different channels
	min_severity[ZST_LOG_ENTITY_CHANNEL] = level;
	min_severity[ZST_LOG_NET_CHANNEL] = level;
	min_severity[ZST_LOG_APP_CHANNEL] = level;
	logging::core::get()->add_global_attribute("ProcessName", attrs::current_process_name());

	sink->set_formatter(expr::stream << "[" << process_name << "] " << line_id << ": <" << severity << "> [" << channel << "] " << expr::smessage);
	sink->set_filter(min_severity || severity >= error);
	logging::core::get()->add_sink(sink);
	logging::add_common_attributes();
}

void ZstLog::init_file_logging(const char * log_file_path)
{
	typedef sinks::synchronous_sink<sinks::text_ostream_backend> text_sink;	boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();
	// Add a stream to write log to
	sink->locked_backend()->add_stream(boost::make_shared<std::ofstream>(log_file_path));
	// Register the sink in the logging core
	logging::core::get()->add_sink(sink);
}

void ZstLog::internals::entity_sink_message(LogLevel level, const char* msg)
{
	BOOST_LOG_CHANNEL_SEV(ZstGlobalLogger::get(), ZST_LOG_ENTITY_CHANNEL, level) << msg;
}

void ZstLog::internals::net_sink_message(LogLevel level, const char* msg)
{
	BOOST_LOG_CHANNEL_SEV(ZstGlobalLogger::get(), ZST_LOG_NET_CHANNEL, level) << msg;
}

void ZstLog::internals::app_sink_message(LogLevel level, const char* msg)
{
	BOOST_LOG_CHANNEL_SEV(ZstGlobalLogger::get(), ZST_LOG_APP_CHANNEL, level) << msg;
}

#ifdef WIN32
WORD get_colour(LogLevel level){
	switch (level){
	case LogLevel::debug: return DEBUG_COLOUR;
	case LogLevel::notification: return NOTIF_COLOUR;
	case LogLevel::warn: return WARN_COLOUR;
	case LogLevel::error: return ERROR_COLOUR;
	default: return 0x0F;
	}
}
void ZstLog::internals::coloured_console_sink::consume(boost::log::record_view const& rec, string_type const& formatted_string){
	auto level = rec.attribute_values()["Severity"].extract<LogLevel>();
	auto channel = rec.attribute_values()["Channel"].extract<std::string>();
	auto hstdout = GetStdHandle(STD_OUTPUT_HANDLE);	CONSOLE_SCREEN_BUFFER_INFO csbi;	GetConsoleScreenBufferInfo(hstdout, &csbi);	SetConsoleTextAttribute(hstdout, get_colour(level.get()));	std::cout << formatted_string << std::endl;	SetConsoleTextAttribute(hstdout, csbi.wAttributes);
}
#endif