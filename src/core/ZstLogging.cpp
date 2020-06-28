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
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/expressions.hpp>
#include <boost/core/null_deleter.hpp>
#include <fstream>

#include <showtime/ZstLogging.h>
#include <showtime/adaptors/ZstLogAdaptor.hpp>
#include "ZstEventDispatcher.hpp"
#include "ZstCorePointerUtils.h"
#ifdef WIN32
#include <windows.h>
#endif

// Boost namespaces 
namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;


// Logging napespace
namespace showtime {

	// Log attributes
	BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
		BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", showtime::Log::Level)
		BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string)
		BOOST_LOG_ATTRIBUTE_KEYWORD(process_name, "ProcessName", std::string)
		BOOST_LOG_ATTRIBUTE_KEYWORD(thread_id, "ThreadID", attrs::current_thread_id::value_type)
		//BOOST_LOG_ATTRIBUTE_KEYWORD(thread_id, "Thread", std::string)

		// Global logger
		typedef src::severity_channel_logger_mt<Log::Level, std::string> ZstLogger_mt;
	BOOST_LOG_GLOBAL_LOGGER(ZstGlobalLogger, ZstLogger_mt)

		// Global logger init
		BOOST_LOG_GLOBAL_LOGGER_INIT(ZstGlobalLogger, ZstLogger_mt) {
		ZstLogger_mt logger = ZstLogger_mt();
		return logger;
	}

	// Internal namespace extension
	namespace Log {
		namespace internals {
			// Custom Windows sink
			class SinkBackend : 
				public boost::log::sinks::text_ostream_backend
			{
			public:
				SinkBackend(std::shared_ptr<ZstEventDispatcher< std::shared_ptr<ZstLogAdaptor> > >& log_events);

				// TODO: Does consume need to be static?
				void consume(boost::log::record_view const& rec, string_type const& formatted_string);
				
				std::shared_ptr<ZstEventDispatcher< std::shared_ptr<ZstLogAdaptor> > >& log_events();

			private:
				std::shared_ptr<ZstEventDispatcher< std::shared_ptr<ZstLogAdaptor> > > m_log_events;
			};
		}
	}

	void my_formatter(logging::record_view const& rec, logging::formatting_ostream& strm)
	{
		//expr::stream << "[" << process_name << "] " << line_id << ": <" << severity << "> [" << channel << "] " << expr::smessage
		strm << logging::extract< unsigned int >("LineID", rec) << ": ";
		strm << "[" << logging::extract<std::string>("ProcessName", rec) << "] ";
		strm << "[" << rec[thread_id] << "] ";
		strm << "[" << logging::extract<std::string>("Channel", rec) << "] ";
		strm << "<" << get_severity_str(logging::extract<Log::Level>("Severity", rec).get()) << "> ";
		strm << rec[expr::smessage];
	}

	void Log::init_logger(const char* logger_name, Log::Level level, std::shared_ptr<ZstEventDispatcher< std::shared_ptr<ZstLogAdaptor> > >& log_events)
	{
		//Flag logger as already launched
		if (Log::internals::_logging)
			return;
		Log::internals::_logging = true;

		// Create backend and sink
		auto logger_backend = boost::make_shared< showtime::Log::internals::SinkBackend >(log_events);
		typedef boost::log::sinks::synchronous_sink<Log::internals::SinkBackend> SinkBackend_t;
		auto sink = boost::make_shared<SinkBackend_t>(logger_backend);

		// Set up external logging stream
		logger_backend->add_stream(
			boost::shared_ptr< std::ostream >(&std::clog, boost::null_deleter()));
		logger_backend->auto_flush(true);

		typedef expr::channel_severity_filter_actor< std::string, Log::Level > min_severity_filter;
		min_severity_filter min_severity = expr::channel_severity_filter(channel, severity);

		// Set up the minimum severity levels for different channels
		min_severity[ZST_LOG_ENTITY_CHANNEL] = level;
		min_severity[ZST_LOG_NET_CHANNEL] = level;
		min_severity[ZST_LOG_SERVER_CHANNEL] = level;
		min_severity[ZST_LOG_APP_CHANNEL] = level;
		logging::add_common_attributes();
		logging::core::get()->add_global_attribute("ProcessName", attrs::current_process_name());

		sink->set_formatter(&my_formatter);
		sink->set_filter(min_severity || severity >= error);
		logging::core::get()->add_sink(sink);
	}

	void Log::init_file_logging(const char* log_file_path)
	{
		typedef sinks::synchronous_sink<sinks::text_ostream_backend> text_sink;
		boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();

		// Add a stream to write log to
		sink->locked_backend()->add_stream(boost::make_shared<std::ofstream>(log_file_path));

		// Register the sink in the logging core
		logging::core::get()->add_sink(sink);
	}

	const char* Log::get_severity_str(Log::Level level) {
		switch (level) {
		case Log::Level::debug:
			return "DEBUG";
		case Log::Level::error:
			return "ERROR";
		case Log::Level::notification:
			return "NOTIF";
		case Log::Level::Levelsize:
			return "SIZE";
		case Log::Level::warn:
			return "WARN";
		}
		return "";
	}

	void Log::internals::entity_sink_message(Log::Level level, const char* msg)
	{
		BOOST_LOG_CHANNEL_SEV(ZstGlobalLogger::get(), ZST_LOG_ENTITY_CHANNEL, level) << msg;
	}

	void Log::internals::net_sink_message(Log::Level level, const char* msg)
	{
		BOOST_LOG_CHANNEL_SEV(ZstGlobalLogger::get(), ZST_LOG_NET_CHANNEL, level) << msg;
	}

	void Log::internals::server_sink_message(Log::Level level, const char* msg)
	{
		BOOST_LOG_CHANNEL_SEV(ZstGlobalLogger::get(), ZST_LOG_SERVER_CHANNEL, level) << msg;
	}

	void Log::internals::app_sink_message(Log::Level level, const char* msg)
	{
		BOOST_LOG_CHANNEL_SEV(ZstGlobalLogger::get(), ZST_LOG_APP_CHANNEL, level) << msg;
	}

#ifdef WIN32
	WORD get_colour(Log::Level level)
	{
		switch (level) {
		case Log::Level::debug: return DEBUG_COLOUR;
		case Log::Level::notification: return NOTIF_COLOUR;
		case Log::Level::warn: return WARN_COLOUR;
		case Log::Level::error: return ERROR_COLOUR;
		default: return RESET_COLOUR;
		}
	}
#endif

	Log::internals::SinkBackend::SinkBackend(std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstLogAdaptor>>>& log_events) : m_log_events(log_events)
	{
	}

	void Log::internals::SinkBackend::consume(boost::log::record_view const& rec, string_type const& formatted_string)
	{
		auto line_ID = logging::extract< unsigned int >("LineID", rec);
		auto process_name = logging::extract<std::string>("ProcessName", rec);
		
		std::ostringstream thread_stream;
		thread_stream << rec[thread_id];
		auto level = rec.attribute_values()["Severity"].extract<Log::Level>();
		auto channel = rec.attribute_values()["Channel"].extract<std::string>();
		auto message = rec[expr::smessage].get();

#ifdef WIN32
		auto hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(hstdout, &csbi);
		SetConsoleTextAttribute(hstdout, get_colour(level.get()));
#endif

		// Send formatted mesage to stream
		std::cout << formatted_string << std::endl;

		// Queue log message to the log event dispatcher
		if (m_log_events) {
			Record event_record{
				line_ID.get(),
				process_name.get(),
				thread_stream.str(),
				level.get(),
				channel.get(),
				message
			};
			m_log_events->defer([event_record](std::shared_ptr<ZstLogAdaptor> adp){ 
				adp->on_log_record(event_record); 
			});
		}
#ifdef WIN32
		SetConsoleTextAttribute(hstdout, csbi.wAttributes);
#endif
	}
	std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstLogAdaptor>>>& Log::internals::SinkBackend::log_events()
	{
		return m_log_events;
	}
}