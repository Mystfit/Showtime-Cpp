#include <string>
#include <iostream>
#include "ZstInstrument.h"
#include "ZstSection.h"
#include "ZstPlug.h"

using namespace Showtime;

int main(int argc,char **argv){
	
	unique_ptr<ZstSection> section = ZstSection::create_section("test_section");
	shared_ptr<ZstInstrument> instrument = section->create_instrument("test_instrument");
	shared_ptr<ZstPlug> plug_out = instrument->create_plug("test_readable_plug", ZstPlug::PlugMode::READABLE);
	shared_ptr<ZstPlug> plug_in = instrument->create_plug("test_writable_plug", ZstPlug::PlugMode::WRITEABLE);

	cout << instrument->get_name() << endl;
	cout << plug_out->get_name() << endl;
	cout << plug_in->get_name() << endl;
	system("pause");


	return 0;
}