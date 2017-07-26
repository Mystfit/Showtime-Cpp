#include "Showtime.h"
#include "version.h"
#include "ZstEndpoint.h"
#include "ZstMessages.h"
#include "ZstPerformer.h"
#include "ZstPlug.h"
#include "ZstEvent.h"
#include "entities\ZstFilter.h"

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
	while (Showtime::event_queue_size() > 0) {
		ZstFilter * filter = NULL;
		ZstCable * cable = NULL;
		ZstEvent e = Showtime::pop_event();

		switch (e.get_update_type()) {
		case ZstEvent::EventType::PLUG_HIT:
			filter = dynamic_cast<ZstFilter*>(Showtime::endpoint().get_entity_by_URI(e.get_first()));
			if (filter != NULL) {
				ZstInputPlug * plug = (ZstInputPlug*)filter->get_plug_by_URI(e.get_first());
				if (plug != NULL) {
					plug->input_events()->run_event_callbacks(plug);
				}
			}
			break;
		case ZstEvent::CABLE_CREATED:
			Showtime::endpoint().cable_arriving_events()->run_event_callbacks(ZstCable(e.get_first(), e.get_second()));
			break;
		case ZstEvent::CABLE_DESTROYED:
			cable = Showtime::endpoint().get_cable_by_URI(e.get_first(), e.get_second());
			if (cable != NULL) {
				Showtime::endpoint().remove_cable(cable);
				Showtime::endpoint().cable_leaving_events()->run_event_callbacks(ZstCable(e.get_first(), e.get_second()));
			}
			break;
		case ZstEvent::PLUG_CREATED:
			Showtime::endpoint().plug_arriving_events()->run_event_callbacks(e.get_first());
			break;
		case ZstEvent::PLUG_DESTROYED:
			Showtime::endpoint().plug_leaving_events()->run_event_callbacks(e.get_first());
			break;
		case ZstEvent::ENTITY_CREATED:
			Showtime::endpoint().entity_arriving_events()->run_event_callbacks(e.get_first());
			break;
		case ZstEvent::ENTITY_DESTROYED:
			Showtime::endpoint().entity_arriving_events()->run_event_callbacks(e.get_first());
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

void Showtime::attach_performer_arriving_callback(ZstEntityEventCallback * callback)
{
	Showtime::endpoint().entity_arriving_events()->attach_event_callback(callback);
}

void Showtime::attach_performer_leaving_callback(ZstEntityEventCallback * callback)
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

void Showtime::remove_stage_event_callback(ZstEventCallback * callback)
{
	Showtime::endpoint().stage_events()->remove_event_callback(callback);
}

void Showtime::remove_performer_arriving_callback(ZstEntityEventCallback * callback)
{
	Showtime::endpoint().entity_arriving_events()->remove_event_callback(callback);
}

void Showtime::remove_performer_leaving_callback(ZstEntityEventCallback * callback)
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

int Showtime::destroy_entity(ZstEntityBase * entity)
{
	return Showtime::endpoint().destroy_entity(entity);
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
