#include "Showtime.h"
#include "version.h"
#include "ZstEndpoint.h"
#include "ZstMessages.h"
#include "ZstPerformer.h"
#include "ZstPlug.h"
#include "ZstEvent.h"
#include "entities/ZstFilter.h"

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
	cout << "Starting Showtime v" << SHOWTIME_VERSION << endl;
	Showtime::endpoint().init();
}

void Showtime::join(const char * stage_address){
	cout << "ZST: Connecting to stage located at " << stage_address << endl;
	Showtime::endpoint().register_endpoint_to_stage(stage_address);
}

void Showtime::leave()
{
	return Showtime::endpoint().leave_stage();
}

bool Showtime::is_connected()
{
	return Showtime::endpoint().is_connected_to_stage();
}

void Showtime::poll_once()
{
	Showtime::endpoint().process_callbacks();
}


// -----------------
// Callback managers
// -----------------
void Showtime::attach_entity_arriving_callback(ZstEntityEventCallback * callback)
{
	Showtime::endpoint().entity_arriving_events()->attach_event_callback(callback);
}

void Showtime::attach_entity_leaving_callback(ZstEntityEventCallback * callback)
{
	return Showtime::endpoint().entity_leaving_events()->attach_event_callback(callback);
}

void Showtime::attach_plug_arriving_callback(ZstPlugEventCallback * callback)
{
	return Showtime::endpoint().plug_arriving_events()->attach_event_callback(callback);
}

void Showtime::attach_plug_leaving_callback(ZstPlugEventCallback * callback)
{
	return Showtime::endpoint().plug_leaving_events()->attach_event_callback(callback);
}

void Showtime::attach_cable_arriving_callback(ZstCableEventCallback * callback)
{
	return Showtime::endpoint().cable_arriving_events()->attach_event_callback(callback);
}

void Showtime::attach_cable_leaving_callback(ZstCableEventCallback * callback)
{
	return Showtime::endpoint().cable_leaving_events()->attach_event_callback(callback);
}

//-----

void Showtime::remove_entity_arriving_callback(ZstEntityEventCallback * callback)
{
	Showtime::endpoint().entity_arriving_events()->remove_event_callback(callback);
}

void Showtime::remove_entity_leaving_callback(ZstEntityEventCallback * callback)
{
	return Showtime::endpoint().entity_leaving_events()->remove_event_callback(callback);
}

void Showtime::remove_plug_arriving_callback(ZstPlugEventCallback * callback)
{
	return Showtime::endpoint().plug_arriving_events()->remove_event_callback(callback);
}

void Showtime::remove_plug_leaving_callback(ZstPlugEventCallback * callback)
{
	return Showtime::endpoint().plug_leaving_events()->remove_event_callback(callback);
}

void Showtime::remove_cable_arriving_callback(ZstCableEventCallback * callback)
{
	return Showtime::endpoint().cable_arriving_events()->remove_event_callback(callback);
}

void Showtime::remove_cable_leaving_callback(ZstCableEventCallback * callback)
{
	return Showtime::endpoint().cable_leaving_events()->remove_event_callback(callback);
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

int Showtime::ping_stage(){
	return Showtime::endpoint().ping_stage();
}

void Showtime::register_entity_type(const char * entity_type)
{
	Showtime::endpoint().register_entity_type(entity_type);
}

ZstEvent * Showtime::pop_event()
{
	return Showtime::endpoint().pop_event();
}

int Showtime::event_queue_size()
{
	return Showtime::endpoint().event_queue_size();
}

int Showtime::connect_cable(ZstURI a, ZstURI b)
{
	return Showtime::endpoint().connect_cable(a, b);
}

int Showtime::destroy_cable(ZstURI a, ZstURI b)
{
	return Showtime::endpoint().destroy_cable(a, b);
}
