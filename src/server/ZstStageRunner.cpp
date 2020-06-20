#include "showtime/ZstVersion.h"
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <cxxopts.hpp>
#include <ShowtimeServer.h>
#include <showtime/ZstLogging.h>

using namespace showtime;

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <signal.h>
#endif

#ifdef WIN32
#define TAKE_A_BREATH Sleep(100);
#else
#define TAKE_A_BREATH usleep(1000 * 200);
#endif

static int s_interrupted = 0;

#ifdef WIN32
static bool s_signal_handler(DWORD signal_value)
#else
static void s_signal_handler(int signal_value)
#endif
{
	Log::server(Log::Level::debug, "Caught signal {}", signal_value);
	switch (signal_value) {
#ifdef WIN32
	case CTRL_C_EVENT:
		s_interrupted = 1;
		return true;
	case CTRL_CLOSE_EVENT:
		s_interrupted = 1;
		return true;
	default:
		break;
	}
	return false;
#else
	case SIGINT:
		s_interrupted = 1;
	case SIGTERM:
		s_interrupted = 1;
	case SIGKILL:
		s_interrupted = 1;
	case SIGABRT:
		s_interrupted = 1;
	default:
		break;
	}
#endif
}

static void s_catch_signals(){
#ifdef WIN32
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)s_signal_handler, TRUE)) {
		Log::server(Log::Level::error, "Unable to register Control Handler");
	}
#else
	struct sigaction action;
	action.sa_handler = s_signal_handler;
	action.sa_flags = 0;
	sigemptyset(&action.sa_mask);
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGTERM, &action, NULL);
#endif
}

int main(int argc, char **argv)
{
	// CLI options
	cxxopts::Options options("ShowtimeServer", "Standalone Showtime server for hosting a performance");
	options.add_options()
		("v,verbose", "Verbose logging")
		("l,log_file", "Log to file", cxxopts::value<std::string>())
		("n,name", "Server name", cxxopts::value<std::string>())
		("unlisted", "Disable server address broadcasting")
		("p,port", "Port", cxxopts::value<int>());
	auto opts = options.parse(argc, argv);

	std::string server_name = "stage";
	int server_port = -1;
	auto server_log_level = Log::Level::notification;
	bool server_unlisted = false;
	try {
		if (opts["name"].count() > 0)
			server_name = opts["name"].as<std::string>();
		if (opts["port"].count() > 0)
			server_port = opts["port"].as<int>();
		if ((opts["v"].count() > 0))
			server_log_level = Log::Level::debug;
		if ((opts["unlisted"].count() > 0))
			server_unlisted = true;
		if ((opts["log_file"].count() > 0))
			Log::init_file_logging(opts["log_file"].as<std::string>().c_str());
	}
	catch (cxxopts::OptionParseException) {
		std::cout << "Could not parse arguments" << std::endl;
		return 1;
	}

	Log::server(Log::Level::notification, "Starting Showtime v{} stage server", SHOWTIME_VERSION_STRING);
	Log::init_logger(server_name.c_str(), server_log_level);
	auto server = std::make_shared<ShowtimeServer>(server_name, server_port, server_unlisted);

	if (argc < 2) {
		Log::server(Log::Level::notification, "Stage running in standalone mode. Press Ctrl+C to exit");
		s_catch_signals();
		while(!s_interrupted){
			TAKE_A_BREATH
		}
	}
	else {
		if (strcmp(argv[1], "-t") == 0)
		{
			Log::server(Log::Level::notification, "Stage running in test mode. Waiting for $TERM on stdin");
			std::string line;
			do {
				std::getline(std::cin, line);
				TAKE_A_BREATH
			} while (line != "$TERM");
			Log::server(Log::Level::notification, "Received $TERM. Closing stage server.");
		}
	}

	Log::app(Log::Level::notification, "Server shutting down");
	server->destroy();

	return 0;
}
