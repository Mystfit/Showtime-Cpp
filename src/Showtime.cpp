#include "Showtime.h"
#include "ZstEndpoint.h"
#include "ZstMessages.h"
#include "ZstPlug.h"

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

void Showtime::destroy() {
	Showtime::endpoint().destroy();
}

void Showtime::init()
{
	Showtime::endpoint().init();
}

void Showtime::join(const char * stage_address){
	cout << "Connecting to stage located at " << stage_address << endl;
	Showtime::endpoint().register_endpoint_to_stage(stage_address);
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

ZstIntPlug * Showtime::create_int_plug(ZstURI * uri) {
	return Showtime::endpoint().create_int_plug(uri);
}

ZstPerformer * Showtime::get_performer_by_name(const char * performer)
{
	return Showtime::endpoint().get_performer_by_name(performer);
}

PlugEvent Showtime::pop_plug_event()
{
	return Showtime::endpoint().pop_plug_event();
}

int Showtime::plug_event_queue_size()
{
	return Showtime::endpoint().plug_event_queue_size();
}

void Showtime::destroy_plug(ZstPlug * plug)
{
	return Showtime::endpoint().destroy_plug(plug);
}

std::vector<ZstURI> Showtime::get_all_plug_addresses(const char * performer, const char * instrument){
	return Showtime::endpoint().get_all_plug_addresses(performer, instrument);
}

void Showtime::connect_plugs(const ZstURI * a, const ZstURI * b)
{
	return Showtime::endpoint().connect_plugs(a, b);
}
