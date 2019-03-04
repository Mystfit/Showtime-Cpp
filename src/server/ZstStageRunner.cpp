#include "ZstVersion.h"
#include <stdio.h>
#include <iostream>
#include <ShowtimeServer.h>
#include "ZstLogging.h"

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <signal.h>
#endif

//Standalone stage runner
//-----------------------

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
	ZstLog::app(LogLevel::debug, "Caught signal {}", signal_value);
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
		ZstLog::app(LogLevel::error, "Unable to register Control Handler");
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
	ZstLog::net(LogLevel::notification, "Starting Showtime v{} stage server", SHOWTIME_VERSION);
	ZstLog::init_logger("stage", LogLevel::debug);
	ZstLog::init_file_logging("server.log");

	auto stage = zst_create_server("stage");

	if (argc < 2) {
		ZstLog::app(LogLevel::notification, "Stage running in standalone mode. Press Ctrl+C to exit");
		s_catch_signals();
		while(!s_interrupted){
			TAKE_A_BREATH
		}
	}
	else {
		if (strcmp(argv[1], "-t") == 0)
		{
			ZstLog::app(LogLevel::notification, "Stage running in test mode. Waiting for $TERM on stdin");
			std::string line;
			do {
				std::getline(std::cin, line);
				TAKE_A_BREATH
			} while (line != "$TERM");
			ZstLog::app(LogLevel::notification, "Received $TERM. Closing stage server.");
		}
	}

	std::cout << "Showtime Stage shutting down" << std::endl;
	zst_destroy_server(stage);

	return 0;
}
