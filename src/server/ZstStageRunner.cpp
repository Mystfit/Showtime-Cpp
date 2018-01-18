#include "ZstStage.h"
#include "ZstVersion.h"

//Standalone stage runner
//-----------------------
int main(int argc, char **argv) {

	ZstStage stage;
	stage.init();

#ifdef WIN32
	system("pause");
#else
	system("read -n 1 -s -p \"Press any key to continue...\"");
#endif
	
	return 0;
}