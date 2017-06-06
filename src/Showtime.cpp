#include "Showtime.h"
#include "ZstEndpoint.h"
#include "ZstMessages.h"

using namespace std;

RuntimeLanguage Showtime::_runtime_language = RuntimeLanguage::NATIVE_RUNTIME;

Showtime::Showtime(){
}

Showtime::~Showtime(){
}

ZstEndpoint & Showtime::endpoint()
{
	static ZstEndpoint endpoint_singleton;
	return endpoint_singleton;
}

void Showtime::join(const char * stage_address){
	cout << "Connecting to stage located at " << stage_address << endl;
	Showtime::endpoint().init(stage_address);
	Showtime::endpoint().register_endpoint_to_stage();
}

void Showtime::set_runtime_language(RuntimeLanguage runtime)
{
	_runtime_language = runtime;
}

RuntimeLanguage Showtime::get_runtime_language()
{
	return _runtime_language;
}


// -------------
// API Functions
// -------------

std::chrono::milliseconds Showtime::ping_stage(){
	return Showtime::endpoint().ping_stage();
}

ZstPerformer * Showtime::create_performer(const char * name)
{
	return Showtime::endpoint().create_performer(name);
}

ZstPerformer * Showtime::get_performer_by_name(const char * performer)
{
	return Showtime::endpoint().get_performer_by_name(performer);
}

void Showtime::destroy_plug(ZstPlug * plug)
{
	return Showtime::endpoint().destroy_plug(plug);
}

std::vector<ZstURI> Showtime::get_all_plug_addresses(const char * performer, const char * instrument){
	return Showtime::endpoint().get_all_plug_addresses(performer, instrument);
}

void Showtime::connect_plugs(ZstURI a, ZstURI b)
{
	return Showtime::endpoint().connect_plugs(a, b);
}
