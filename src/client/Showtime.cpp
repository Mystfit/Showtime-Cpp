#include <ZstVersion.h>
#include <ZstCable.h>
#include <ZstEvents.h>
#include <entities/ZstEntityBase.h>
#include <entities/ZstPlug.h>
#include <entities/ZstComponent.h>

#include "Showtime.h"
#include "ZstClient.h"

using namespace std;

Showtime::Showtime(){
}

Showtime::~Showtime(){
}


// -----------------
// Initialisation
// -----------------

void Showtime::init(const char * performer_name)
{
	ZstClient::instance().init(performer_name);
}

void Showtime::join(const char * stage_address){
	ZstClient::instance().register_client_to_stage(stage_address);
}


// -----------------
// Cleanup
// -----------------

void Showtime::destroy() {
	ZstClient::instance().destroy();
}

void Showtime::leave()
{
	return ZstClient::instance().leave_stage();
}


// -----------------
// Event polling
// -----------------

void Showtime::poll_once()
{
	ZstClient::instance().process_callbacks();
}


// -----------------
// Callbacks
// -----------------

void Showtime::attach_event_listener(ZstClientConnectionEvent * callback)
{
	ZstClient::instance().client_connected_events()->attach_event_listener(callback);
}

void Showtime::attach_event_listener(ZstPerformerEvent * callback, ZstEventAction action)
{
	if (action == ZstEventAction::ARRIVING) {
		ZstClient::instance().performer_arriving_events()->attach_event_listener(callback);
	}
	else if (action == ZstEventAction::LEAVING) {
		ZstClient::instance().performer_leaving_events()->attach_event_listener(callback);
	}
}

void Showtime::attach_event_listener(ZstComponentEvent * callback, ZstEventAction action)
{
    if(action == ZstEventAction::ARRIVING){
        ZstClient::instance().entity_arriving_events()->attach_event_listener(callback);
    } else if(action == ZstEventAction::LEAVING){
        ZstClient::instance().entity_leaving_events()->attach_event_listener(callback);
    }
}

void Showtime::attach_event_listener(ZstComponentTypeEvent * callback, ZstEventAction action)
{
    if(action == ZstEventAction::ARRIVING){
        ZstClient::instance().component_type_arriving_events()->attach_event_listener(callback);
    } else if(action == ZstEventAction::LEAVING){
        ZstClient::instance().component_type_leaving_events()->attach_event_listener(callback);
    }
}

void Showtime::attach_event_listener(ZstPlugEvent * callback, ZstEventAction action)
{
    if(action == ZstEventAction::ARRIVING){
        ZstClient::instance().plug_arriving_events()->attach_event_listener(callback);
    } else if(action == ZstEventAction::LEAVING){
        ZstClient::instance().plug_leaving_events()->attach_event_listener(callback);
    }
}
void Showtime::attach_event_listener(ZstCableEvent * callback, ZstEventAction action)
{
    if(action == ZstEventAction::ARRIVING){
        ZstClient::instance().cable_arriving_events()->attach_event_listener(callback);
    } else if(action == ZstEventAction::LEAVING){
        ZstClient::instance().cable_leaving_events()->attach_event_listener(callback);
    }
}

void Showtime::remove_event_listener(ZstClientConnectionEvent * callback)
{
	ZstClient::instance().client_connected_events()->attach_event_listener(callback);
}

void Showtime::remove_event_listener(ZstPerformerEvent * callback, ZstEventAction action)
{
	if (action == ZstEventAction::ARRIVING) {
		ZstClient::instance().performer_arriving_events()->remove_event_listener(callback);
	}
	else if (action == ZstEventAction::LEAVING) {
		ZstClient::instance().performer_arriving_events()->remove_event_listener(callback);
	}
}

void Showtime::remove_event_listener(ZstComponentEvent * callback, ZstEventAction action)
{
    if(action == ZstEventAction::ARRIVING){
        ZstClient::instance().entity_arriving_events()->remove_event_listener(callback);
    } else if(action == ZstEventAction::LEAVING){
        ZstClient::instance().entity_leaving_events()->remove_event_listener(callback);
    }
}

void Showtime::remove_event_listener(ZstComponentTypeEvent * callback, ZstEventAction action)
{
    if(action == ZstEventAction::ARRIVING){
        ZstClient::instance().component_type_arriving_events()->remove_event_listener(callback);
    } else if(action == ZstEventAction::LEAVING){
        ZstClient::instance().component_type_leaving_events()->remove_event_listener(callback);
    }
}

void Showtime::remove_event_listener(ZstPlugEvent * callback, ZstEventAction action)
{
    if(action == ZstEventAction::ARRIVING){
		ZstClient::instance().plug_arriving_events()->remove_event_listener(callback);
    } else if(action == ZstEventAction::LEAVING){
        ZstClient::instance().plug_leaving_events()->remove_event_listener(callback);
    }
}

void Showtime::remove_event_listener(ZstCableEvent * callback, ZstEventAction action)
{
    if(action == ZstEventAction::ARRIVING){
		ZstClient::instance().cable_arriving_events()->remove_event_listener(callback);
    } else if(action == ZstEventAction::LEAVING){
		ZstClient::instance().cable_leaving_events()->remove_event_listener(callback);
    }
}


// -----------------------
// Entity activation/deactivation
// -----------------------
void Showtime::activate(ZstEntityBase * entity)
{
	ZstClient::instance().activate_entity(entity);
}

void Showtime::deactivate(ZstEntityBase * entity)
{
	ZstClient::instance().destroy_entity(entity);
}


// -------------
// Hierarchy
// -------------

ZstPerformer * Showtime::get_root()
{
	return ZstClient::instance().get_local_performer();
}

ZstPerformer * Showtime::get_performer_by_URI(const ZstURI & path)
{
    return ZstClient::instance().get_performer_by_URI(path);
}

ZstEntityBase* Showtime::find_entity(const ZstURI & path)
{
	return ZstClient::instance().find_entity(path);
}


// -------------
// Stage status
// -------------

bool Showtime::is_connected()
{
	return ZstClient::instance().is_connected_to_stage();
}

int Showtime::ping()
{
	return ZstClient::instance().ping();
}


// -------------
// Cables
// -------------

ZstCable * Showtime::connect_cable(ZstPlug * a, ZstPlug * b)
{
	return ZstClient::instance().connect_cable(a, b);
}

void Showtime::destroy_cable(ZstCable * cable)
{
	ZstClient::instance().destroy_cable(cable);
}

void Showtime::disconnect_plug(ZstPlug * plug)
{
	ZstClient::instance().disconnect_plug(plug);
}


// -------------
// Creatables
// -------------

void Showtime::register_component_type(ZstComponent * component_template)
{
    ZstClient::instance().register_component_type(component_template);
}

void Showtime::unregister_component_type(ZstComponent * component_template)
{

}

void Showtime::run_component_template(ZstComponent * component)
{
    run_component_template(component, false);
}

void Showtime::run_component_template(ZstComponent * component, bool wait)
{
    ZstClient::instance().run_component_template(component);
}
