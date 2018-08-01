#include "ZstStage.h"
#include "ZstVersion.h"
#include <stdio.h>
#include <signal.h>

//Standalone stage runner
//-----------------------

#ifdef WIN32
#define TAKE_A_BREATH Sleep(100);
#else
#define TAKE_A_BREATH usleep(1000 * 200);
#endif

static int s_interrupted = 0;
static void s_signal_handler(int signal_value)
{
	ZstLog::app(LogLevel::notification, "Caught signal {}", signal_value);
	s_interrupted = 1;
}

static void s_catch_signals(){
	struct sigaction action;
	action.sa_handler = s_signal_handler;
	action.sa_flags = 0;
	sigemptyset(&action.sa_mask);
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGTERM, &action, NULL);
}

int main(int argc, char **argv) {

#ifdef WIN32
	Sleep(100);
#else
    usleep(1000 * 200);
#endif

	ZstStage stage;
	stage.init_stage("stage", true);

	if (argc < 2) {
		ZstLog::app(LogLevel::notification, "Stage running in standalone mode. Press Ctrl+C to exit");
#ifdef WIN32
		system("pause");
#else
		s_catch_signals();
		while(!s_interrupted){
			TAKE_A_BREATH
		}
#endif
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
