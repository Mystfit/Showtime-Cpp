#include "ZstStage.h"
#include "version.h"
#include "Showtime.h"

//Standalone stage runner
//-----------------------
int main(int argc, char **argv) {

	std::cout << "Starting Showtime v" << SHOWTIME_VERSION << " stage server" << std::endl;
	ZstStage * stage = ZstStage::create_stage();

#ifdef WIN32
	system("pause");
#else
	system("read -n 1 -s -p \"Press any key to continue...\"");
#endif

	delete stage;
	Showtime::destroy();
	return 0;
}