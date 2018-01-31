#include "ZstStage.h"
#include "ZstVersion.h"
#include <stdio.h>

//Standalone stage runner
//-----------------------
int main(int argc, char **argv) {

#ifdef WIN32
	Sleep(100);
#else
    usleep(1000 * 200);
#endif

	ZstStage stage;
	stage.init();

#ifdef WIN32
	system("pause");
#else
	system("read -n 1 -s -p \"Press any key to continue...\n\"");
#endif

	stage.destroy();

	std::cout << "Showtime Stage sut down successfully" << std::endl;

	return 0;
}
