#include "Showtime.h"
#include "ZstEndpoint.h"
#include "ZstMessages.h"
#include "ZstPerformer.h"
#include "ZstPlug.h"
#include "ZstEvent.h"

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

void Showtime::poll_once()
{
	while (Showtime::event_queue_size() > 0) {
		ZstEvent e = Showtime::pop_event();
		Showtime::endpoint().get_performer_by_URI(e.get_first().to_str())->get_plug_by_URI(e.get_first().to_str())->run_recv_callbacks();
	}
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

ZstPerformer * Showtime::get_performer_by_URI(const char * uri_str)
{
	return Showtime::endpoint().get_performer_by_URI(uri_str);
}


ZstEvent Showtime::pop_event()
{
	return Showtime::endpoint().pop_plug_event();
}

int Showtime::event_queue_size()
{
	return Showtime::endpoint().plug_event_queue_size();
}

void Showtime::destroy_plug(ZstPlug * plug)
{
	return Showtime::endpoint().destroy_plug(plug);
}

void Showtime::connect_plugs(const ZstURI * a, const ZstURI * b)
{
	return Showtime::endpoint().connect_plugs(a, b);
}
