#include "ZstStage.h"
#include "ZstVersion.h"
#include <stdio.h>

#ifdef WIN32
#include <windows.h>
#else
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

static bool s_signal_handler(unsigned long signal_value)
{
	ZstLog::app(LogLevel::debug, "Caught signal {}", signal_value);
#ifdef WIN32
	switch (signal_value) {
	case CTRL_C_EVENT:
		s_interrupted = 1;
		return true;
	case CTRL_CLOSE_EVENT:
		s_interrupted = 1;
		return true;
#else
	case SIGINT:
		s_interrupted = 1;
		return true;
	case SIGTERM:
		s_interrupted = 1;
		return true;
	case SIGKILL:
		s_interrupted = 1;
		return true;
	case SIGABRT:
		s_interrupted = 1;
		return true;
#endif
	default:
		break;
	}
	return false;
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
	ZstStage stage;
	stage.init_stage("stage", true);

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
	stage.destroy();

	return 0;
}
