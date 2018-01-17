#include "Showtime.h"
#include "ZstClient.h"
#include "ZstVersion.h"
#include "ZstCable.h"
#include "ZstCallbacks.h"
#include "entities/ZstEntityBase.h"
#include "entities/ZstPlug.h"
#include "entities/ZstComponent.h"

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
	cout << "Starting Showtime v" << SHOWTIME_VERSION << endl;
	ZstClient::instance().init(performer_name);
}

void Showtime::join(const char * stage_address){
	cout << "ZST: Connecting to stage located at " << stage_address << endl;
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

void Showtime::attach(ZstComponentEvent * callback, ZstCallbackAction action)
{
    if(action == ZstCallbackAction::ARRIVING){
        return ZstClient::instance().component_arriving_events()->attach_event_callback(callback);
    } else if(action == ZstCallbackAction::LEAVING){
        return ZstClient::instance().component_leaving_events()->attach_event_callback(callback);
    }
}

void Showtime::attach(ZstComponentTypeEvent * callback, ZstCallbackAction action)
{
    if(action == ZstCallbackAction::ARRIVING){
        return ZstClient::instance().component_type_arriving_events()->attach_event_callback(callback);
    } else if(action == ZstCallbackAction::LEAVING){
        return ZstClient::instance().component_type_leaving_events()->attach_event_callback(callback);
    }
}

void Showtime::attach(ZstPlugEvent * callback, ZstCallbackAction action)
{
    if(action == ZstCallbackAction::ARRIVING){
        return ZstClient::instance().plug_arriving_events()->attach_event_callback(callback);
    } else if(action == ZstCallbackAction::LEAVING){
        return ZstClient::instance().plug_leaving_events()->attach_event_callback(callback);
    }
}
void Showtime::attach(ZstCableEvent * callback, ZstCallbackAction action)
{
    if(action == ZstCallbackAction::ARRIVING){
        return ZstClient::instance().cable_arriving_events()->attach_event_callback(callback);
    } else if(action == ZstCallbackAction::LEAVING){
        return ZstClient::instance().cable_leaving_events()->attach_event_callback(callback);
    }
}

void Showtime::detach(ZstComponentEvent * callback, ZstCallbackAction action)
{
    if(action == ZstCallbackAction::ARRIVING){
        ZstClient::instance().component_arriving_events()->remove_event_callback(callback);
    } else if(action == ZstCallbackAction::LEAVING){
        return ZstClient::instance().component_leaving_events()->remove_event_callback(callback);
    }
}

void Showtime::detach(ZstComponentTypeEvent * callback, ZstCallbackAction action)
{
    if(action == ZstCallbackAction::ARRIVING){
        return ZstClient::instance().component_type_arriving_events()->remove_event_callback(callback);
    } else if(action == ZstCallbackAction::LEAVING){
        return ZstClient::instance().component_type_leaving_events()->remove_event_callback(callback);
    }
}

void Showtime::detach(ZstPlugEvent * callback, ZstCallbackAction action)
{
    if(action == ZstCallbackAction::ARRIVING){
        return ZstClient::instance().plug_arriving_events()->remove_event_callback(callback);
    } else if(action == ZstCallbackAction::LEAVING){
        return ZstClient::instance().plug_leaving_events()->remove_event_callback(callback);
    }
}

void Showtime::detach(ZstCableEvent * callback, ZstCallbackAction action)
{
    if(action == ZstCallbackAction::ARRIVING){
        return ZstClient::instance().cable_arriving_events()->remove_event_callback(callback);
    } else if(action == ZstCallbackAction::LEAVING){
        return ZstClient::instance().cable_leaving_events()->remove_event_callback(callback);
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
