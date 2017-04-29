#include <string>
#include <iostream>
#include "ZstInstrument.h"
#include "ZstSection.h"
#include "ZstPlug.h"
#include "ZstStage.h"

using namespace Showtime;

int main(int argc,char **argv){
    
    //Set up
    ZstStage *stage = ZstStage::create_stage();

	ZstSection *section = ZstSection::create_section("test_section_1");
    
    assert(section->ping_stage().count() < 10);
    
    
    //section->register_to_stage();
  

	system("pause");

	return 0;
}
