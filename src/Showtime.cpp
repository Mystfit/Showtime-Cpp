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
	cout << "ZST: Connecting to stage located at " << stage_address << endl;
	Showtime::endpoint().register_endpoint_to_stage(stage_address);
}

void Showtime::leave()
{
	return Showtime::endpoint().leave_stage();
}

void Showtime::poll_once()
{
	while (Showtime::event_queue_size() > 0) {
		ZstPerformer * performer = NULL;
		ZstEvent e = Showtime::pop_event();

		switch (e.get_update_type()) {
		case ZstEvent::EventType::PLUG_HIT:
			performer = Showtime::endpoint().get_performer_by_URI(e.get_first());
			if (performer != NULL) {
				//TODO: Verify that plug is actually an input plug!
				ZstInputPlug * plug = (ZstInputPlug*)performer->get_plug_by_URI(e.get_first());
				if (plug != NULL) {
					plug->input_events()->run_event_callbacks(plug);
				}
			}
			break;
		case ZstEvent::CABLE_CREATED:
			Showtime::endpoint().cable_arriving_events()->run_event_callbacks(ZstCable(e.get_first(), e.get_second()));
			break;
		case ZstEvent::CABLE_DESTROYED:
			Showtime::endpoint().cable_leaving_events()->run_event_callbacks(ZstCable(e.get_first(), e.get_second()));
			break;
		case ZstEvent::CREATED:
			Showtime::endpoint().stage_events()->run_event_callbacks(e);
			break;
		case ZstEvent::DESTROYED:
			Showtime::endpoint().stage_events()->run_event_callbacks(e);
			break;
		default:
			Showtime::endpoint().stage_events()->run_event_callbacks(e);
		}
	}
}


// -----------------
// Callback managers
// -----------------
void Showtime::attach_stage_event_callback(ZstEventCallback * callback)
{
	Showtime::endpoint().stage_events()->attach_event_callback(callback);
}

void Showtime::attach_performer_arriving_callback(ZstPerformerEventCallback * callback)
{
	Showtime::endpoint().performer_arriving_events()->attach_event_callback(callback);
}

void Showtime::attach_performer_leaving_callback(ZstPerformerEventCallback * callback)
{
	return Showtime::endpoint().performer_leaving_events()->attach_event_callback(callback);
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

void Showtime::remove_stage_event_callback(ZstEventCallback * callback)
{
	Showtime::endpoint().stage_events()->remove_event_callback(callback);
}

void Showtime::remove_performer_arriving_callback(ZstPerformerEventCallback * callback)
{
	Showtime::endpoint().performer_arriving_events()->remove_event_callback(callback);
}

void Showtime::remove_performer_leaving_callback(ZstPerformerEventCallback * callback)
{
	return Showtime::endpoint().performer_leaving_events()->remove_event_callback(callback);
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

ZstPerformer * Showtime::create_performer(const char * name)
{
	return Showtime::endpoint().create_performer(ZstURI(name, "", ""));
}

ZstInputPlug * Showtime::create_input_plug(ZstURI uri, ZstValueType val_type)
{
	return (ZstInputPlug*)Showtime::endpoint().create_plug<ZstInputPlug>(uri, val_type, PlugDirection::IN_JACK);
}

ZstOutputPlug * Showtime::create_output_plug(ZstURI uri, ZstValueType val_type)
{
    return (ZstOutputPlug*)Showtime::endpoint().create_plug<ZstOutputPlug>(uri, val_type, PlugDirection::OUT_JACK);
}

ZstPerformer * Showtime::get_performer_by_URI(ZstURI uri)
{
	return Showtime::endpoint().get_performer_by_URI(uri);
}

ZstEvent Showtime::pop_event()
{
	return Showtime::endpoint().pop_plug_event();
}

int Showtime::event_queue_size()
{
	return Showtime::endpoint().plug_event_queue_size();
}

int Showtime::destroy_plug(ZstPlug * plug)
{
	return Showtime::endpoint().destroy_plug(plug);
}

int Showtime::connect_cable(ZstURI a, ZstURI b)
{
	return Showtime::endpoint().connect_cable(a, b);
}

int Showtime::destroy_cable(ZstURI a, ZstURI b)
{
	return Showtime::endpoint().destroy_cable(a, b);
}
