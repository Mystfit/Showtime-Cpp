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

		cout << "PERFORMER: Processing event queue. Type is " << e.get_update_type() << ". Uri is " << e.get_first().to_str() << endl;

		if (e.get_update_type() == ZstEvent::EventType::PLUG_HIT)
			Showtime::endpoint().get_performer_by_URI(e.get_first())->get_plug_by_URI(e.get_first())->run_recv_callbacks();
		else
			Showtime::endpoint().run_stage_event_callbacks(e);
	}
}

void Showtime::attach_stage_event_callback(ZstEventCallback * callback) {
	Showtime::endpoint().attach_stage_event_callback(callback);
}

void Showtime::destroy_stage_event_callback(ZstEventCallback * callback) {
	Showtime::endpoint().destroy_stage_event_callback(callback);
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
	return Showtime::endpoint().create_performer(ZstURI(name, "", "", ZstURI::Direction::NONE));
}

ZstIntPlug * Showtime::create_int_plug(ZstURI * uri) {
	return Showtime::endpoint().create_int_plug(uri);
}

ZstPerformer * Showtime::get_performer_by_URI(const ZstURI * uri)
{
	return Showtime::endpoint().get_performer_by_URI(*uri);
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
