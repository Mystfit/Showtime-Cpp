#include "ZstStage.h"

//Standalone stage runner
//-----------------------
int main(int argc, char **argv) {
	ZstStage * stage = ZstStage::create_stage();

	std::cout << "Stage created" << std::endl;

#ifdef WIN32
	system("pause");
#else
	system("read -n 1 -s -p \"Press any key to continue...\"");
#endif

	delete stage;
	Showtime::destroy();
	return 0;
}