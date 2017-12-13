#include "Showtime.h"
#include "version.h"
#include "ZstCable.h"
#include "entities/ZstEntityBase.h"
#include "entities/ZstPlug.h"
#include "entities/ZstComponent.h"
#include "ZstClient.h"
#include "ZstCallbacks.h"

using namespace std;

Showtime::Showtime(){
}

Showtime::~Showtime(){
}

void Showtime::destroy() {
	ZstClient::instance().destroy();
}

void Showtime::init(const char * performer_name)
{
	cout << "Starting Showtime v" << SHOWTIME_VERSION << endl;
	ZstClient::instance().init(performer_name);
}

void Showtime::join(const char * stage_address){
	cout << "ZST: Connecting to stage located at " << stage_address << endl;
	ZstClient::instance().register_client_to_stage(stage_address);
}

void Showtime::leave()
{
	return ZstClient::instance().leave_stage();
}

bool Showtime::is_connected()
{
	return ZstClient::instance().is_connected_to_stage();
}

void Showtime::poll_once()
{
	ZstClient::instance().process_callbacks();
}


// -----------------
// Callback managers
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
// Activation/deactivation
// -----------------------
int Showtime::activate(ZstEntityBase * entity)
{
	return ZstClient::instance().activate_entity(entity);
}

int Showtime::destroy(ZstEntityBase * entity)
{
	return ZstClient::instance().destroy_entity(entity);
}


// -------------
// API Functions
// -------------

ZstContainer * Showtime::get_root()
{
	return ZstClient::instance().get_local_performer();
}

ZstEntityBase * Showtime::get_performer_by_URI(const ZstURI & path)
{
    return ZstClient::instance().get_performer_by_URI(path);
}

int Showtime::ping_stage(){
	return ZstClient::instance().ping_stage();
}

int Showtime::connect_cable(ZstPlug * a, ZstPlug * b)
{
	return ZstClient::instance().connect_cable(a, b);
}

int Showtime::destroy_cable(ZstCable * cable)
{
	return ZstClient::instance().destroy_cable(cable);
}

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


