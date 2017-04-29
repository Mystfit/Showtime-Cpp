#include <string>
#include <iostream>
#include "ZstInstrument.h"
#include "ZstSection.h"
#include "ZstPlug.h"
#include "ZstStage.h"

using namespace Showtime;

int main(int argc,char **argv){
    
    ZstStage *stage = ZstStage::create_stage();

	unique_ptr<ZstSection> section = ZstSection::create_section("test_section");
    //section->register_to_stage();
    

	system("pause");


	return 0;
}
