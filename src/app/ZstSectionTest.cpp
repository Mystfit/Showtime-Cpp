#include <string>
#include <iostream>
#include "ZstInstrument.h"
#include "ZstSection.h"
#include "ZstPlug.h"

using namespace Showtime;

int main(int argc,char **argv){
	
	ZstSection* section = ZstSection::create_section("test_section");
	ZstInstrument* instrument = section->create_instrument("test_instrument");
	ZstPlug* plug_out = instrument->create_plug("test_readable_plug", ZstPlug::PlugMode::READABLE);
	ZstPlug* plug_in = instrument->create_plug("test_writable_plug", ZstPlug::PlugMode::WRITEABLE);

	cout << instrument->get_name() << endl;
	cout << plug_out->get_name() << endl;
	cout << plug_in->get_name() << endl;
	system("pause");


	return 0;
}