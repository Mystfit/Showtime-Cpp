#include "ZstStage.h"
#include "ZstVersion.h"
#include <stdio.h>

//Standalone stage runner
//-----------------------

#ifdef WIN32
#define TAKE_A_BREATH Sleep(100);
#else
#define TAKE_A_BREATH usleep(1000 * 200);
#endif

int main(int argc, char **argv) {

#ifdef WIN32
	Sleep(100);
#else
    usleep(1000 * 200);
#endif

	ZstStage stage;
	stage.init_stage("stage", true);

	if (argc < 2) {
		ZstLog::app(LogLevel::notification, "Stage running in standalone mode");
#ifdef WIN32
		system("pause");
#else
		system("read -n 1 -s -p \"Press any key to continue...\n\"");
#endif
	}
	else {
		if (strcmp(argv[1], "-t") == 0)
		{
			ZstLog::app(LogLevel::notification, "Stage running in test mode. Waiting for $TERM on stdin");
			std::string line;
			do {
				std::getline(std::cin, line);
#ifdef WIN32
				Sleep(100);
#else
				usleep(1000 * 200);
#endif
			} while (line != "$TERM");
			ZstLog::app(LogLevel::notification, "Received $TERM. Closing stage server.");
			}
	}

	std::cout << "Showtime Stage shutting down" << std::endl;

	stage.destroy();

	return 0;
}
