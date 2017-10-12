#include "Showtime.h"
#include "version.h"
#include "ZstEndpoint.h"
#include "ZstMessages.h"
#include "ZstPerformer.h"
#include "ZstPlug.h"
#include "ZstEvent.h"
#include "entities/ZstFilter.h"

using namespace std;

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

void Showtime::attach(ZstEntityEventCallback * callback, ZstCallbackAction action)
{
    if(action == ZstCallbackAction::ARRIVING){
        return Showtime::endpoint().entity_arriving_events()->attach_event_callback(callback);
    } else if(action == ZstCallbackAction::LEAVING){
        return Showtime::endpoint().entity_leaving_events()->attach_event_callback(callback);
    }
}
void Showtime::attach(ZstPlugEventCallback * callback, ZstCallbackAction action)
{
    if(action == ZstCallbackAction::ARRIVING){
        return Showtime::endpoint().plug_arriving_events()->attach_event_callback(callback);
    } else if(action == ZstCallbackAction::LEAVING){
        return Showtime::endpoint().plug_leaving_events()->attach_event_callback(callback);
    }
}
void Showtime::attach(ZstCableEventCallback * callback, ZstCallbackAction action)
{
    if(action == ZstCallbackAction::ARRIVING){
        return Showtime::endpoint().cable_arriving_events()->attach_event_callback(callback);
    } else if(action == ZstCallbackAction::LEAVING){
        return Showtime::endpoint().cable_leaving_events()->attach_event_callback(callback);
    }
}

void Showtime::detach(ZstEntityEventCallback * callback, ZstCallbackAction action)
{
    if(action == ZstCallbackAction::ARRIVING){
        Showtime::endpoint().entity_arriving_events()->remove_event_callback(callback);
    } else if(action == ZstCallbackAction::LEAVING){
        return Showtime::endpoint().entity_leaving_events()->remove_event_callback(callback);
    }
}

void Showtime::detach(ZstPlugEventCallback * callback, ZstCallbackAction action)
{
    if(action == ZstCallbackAction::ARRIVING){
        return Showtime::endpoint().plug_arriving_events()->remove_event_callback(callback);
    } else if(action == ZstCallbackAction::LEAVING){
        return Showtime::endpoint().plug_leaving_events()->remove_event_callback(callback);
    }
}

void Showtime::detach(ZstCableEventCallback * callback, ZstCallbackAction action)
{
    if(action == ZstCallbackAction::ARRIVING){
        return Showtime::endpoint().cable_arriving_events()->remove_event_callback(callback);
    } else if(action == ZstCallbackAction::LEAVING){
        return Showtime::endpoint().cable_leaving_events()->remove_event_callback(callback);
    }
}

// -------------
// API Functions
// -------------

ZstEntityBase * Showtime::get_entity_by_URI(const ZstURI & path)
{
    return Showtime::endpoint().get_entity_by_URI(path);
}

int Showtime::ping_stage(){
	return Showtime::endpoint().ping_stage();
}

ZstEvent * Showtime::pop_event()
{
	return Showtime::endpoint().pop_event();
}

size_t Showtime::event_queue_size()
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

// --

void Showtime::register_composer(ZstComposer *composer){
    Showtime::endpoint().register_entity_type(composer);
}

void Showtime::unregister_composer(ZstComposer *composer){
    Showtime::endpoint().unregister_entity_type(composer);
}

