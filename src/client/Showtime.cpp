#include "Showtime.h"
#include "ZstClient.h"

using namespace std;

// -----------------
// Initialisation
// -----------------

void zst_init(const char * performer_name, bool debug)
{
	ZstClient::instance().init_client(performer_name, debug);
}

void zst_start_file_logging(const char * log_file_path)
{
	ZstClient::instance().init_file_logging(log_file_path);
}

void zst_join(const char * stage_address){
	ZstClient::instance().register_client_to_stage(stage_address, false);
}

void zst_join_async(const char * stage_address){
    ZstClient::instance().register_client_to_stage(stage_address, true);
}


// -----------------
// Cleanup
// -----------------

void zst_destroy() {
	ZstClient::instance().destroy();
}

void zst_leave()
{
    return ZstClient::instance().leave_stage(false);
}

void zst_leave_immediately()
{
	return ZstClient::instance().leave_stage(true);
}


// -----------------
// Event polling
// -----------------

void zst_poll_once()
{
	ZstClient::instance().process_callbacks();
}


// -----------------
// Callbacks
// -----------------

void zst_attach_connection_event_listener(ZstPerformerEvent * callback)
{
	ZstClient::instance().client_connected_events()->attach_event_listener(callback);
	if (ZstClient::instance().is_connected_to_stage())
		callback->cast_run(ZstClient::instance().get_local_performer());
}

void zst_attach_performer_event_listener(ZstPerformerEvent * callback, ZstEventAction action)
{
	if (action == ZstEventAction::ARRIVING) {
		ZstClient::instance().performer_arriving_events()->attach_event_listener(callback);
	}
	else if (action == ZstEventAction::LEAVING) {
		ZstClient::instance().performer_leaving_events()->attach_event_listener(callback);
	}
}

void zst_attach_component_event_listener(ZstComponentEvent * callback, ZstEventAction action)
{
    if(action == ZstEventAction::ARRIVING){
        ZstClient::instance().component_arriving_events()->attach_event_listener(callback);
    } else if(action == ZstEventAction::LEAVING){
        ZstClient::instance().component_leaving_events()->attach_event_listener(callback);
    }
}

void zst_attach_component_type_event_listener(ZstComponentTypeEvent * callback, ZstEventAction action)
{
    if(action == ZstEventAction::ARRIVING){
        ZstClient::instance().component_type_arriving_events()->attach_event_listener(callback);
    } else if(action == ZstEventAction::LEAVING){
        ZstClient::instance().component_type_leaving_events()->attach_event_listener(callback);
    }
}

void zst_attach_plug_event_listener(ZstPlugEvent * callback, ZstEventAction action)
{
    if(action == ZstEventAction::ARRIVING){
        ZstClient::instance().plug_arriving_events()->attach_event_listener(callback);
    } else if(action == ZstEventAction::LEAVING){
        ZstClient::instance().plug_leaving_events()->attach_event_listener(callback);
    }
}
void zst_attach_cable_event_listener(ZstCableEvent * callback, ZstEventAction action)
{
    if(action == ZstEventAction::ARRIVING){
        ZstClient::instance().cable_arriving_events()->attach_event_listener(callback);
    } else if(action == ZstEventAction::LEAVING){
        ZstClient::instance().cable_leaving_events()->attach_event_listener(callback);
    }
}

void zst_remove_connection_event_listener(ZstPerformerEvent * callback)
{
	ZstClient::instance().client_connected_events()->attach_event_listener(callback);
}

void zst_remove_performer_event_listener(ZstPerformerEvent * callback, ZstEventAction action)
{
	if (action == ZstEventAction::ARRIVING) {
		ZstClient::instance().performer_arriving_events()->remove_event_listener(callback);
	}
	else if (action == ZstEventAction::LEAVING) {
		ZstClient::instance().performer_arriving_events()->remove_event_listener(callback);
	}
}

void zst_remove_component_event_listener(ZstComponentEvent * callback, ZstEventAction action)
{
    if(action == ZstEventAction::ARRIVING){
        ZstClient::instance().component_arriving_events()->remove_event_listener(callback);
    } else if(action == ZstEventAction::LEAVING){
        ZstClient::instance().component_leaving_events()->remove_event_listener(callback);
    }
}

void zst_remove_component_type_event_listener(ZstComponentTypeEvent * callback, ZstEventAction action)
{
    if(action == ZstEventAction::ARRIVING){
        ZstClient::instance().component_type_arriving_events()->remove_event_listener(callback);
    } else if(action == ZstEventAction::LEAVING){
        ZstClient::instance().component_type_leaving_events()->remove_event_listener(callback);
    }
}

void zst_remove_plug_event_listener(ZstPlugEvent * callback, ZstEventAction action)
{
    if(action == ZstEventAction::ARRIVING){
		ZstClient::instance().plug_arriving_events()->remove_event_listener(callback);
    } else if(action == ZstEventAction::LEAVING){
        ZstClient::instance().plug_leaving_events()->remove_event_listener(callback);
    }
}

void zst_remove_cable_event_listener(ZstCableEvent * callback, ZstEventAction action)
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
void zst_activate_entity(ZstEntityBase * entity)
{
	ZstClient::instance().activate_entity(entity);
}

void zst_activate_entity_async(ZstEntityBase * entity)
{
    ZstClient::instance().activate_entity(entity, true);
}

void zst_deactivate_entity(ZstEntityBase * entity)
{
	ZstClient::instance().destroy_entity(entity);
}

void zst_deactivate_entity_async(ZstEntityBase * entity)
{
    ZstClient::instance().destroy_entity(entity, true);
}


// -------------
// Hierarchy
// -------------

ZstPerformer * zst_get_root()
{
	return ZstClient::instance().get_local_performer();
}

ZstPerformer * zst_get_performer_by_URI(const ZstURI & path)
{
    return ZstClient::instance().get_performer_by_URI(path);
}

ZstEntityBase* zst_find_entity(const ZstURI & path)
{
	return ZstClient::instance().find_entity(path);
}


// -------------
// Stage status
// -------------

bool zst_is_connected()
{
	return ZstClient::instance().is_connected_to_stage();
}

int zst_ping()
{
	return ZstClient::instance().ping();
}


// -------------
// Cables
// -------------

ZstCable * zst_connect_cable(ZstPlug * input, ZstPlug * output)
{
	return ZstClient::instance().connect_cable(input, output, false);
}

ZstCable * zst_connect_cable_async(ZstPlug * input, ZstPlug * output)
{
    return ZstClient::instance().connect_cable(input, output, true);
}

void zst_destroy_cable(ZstCable * cable)
{
	ZstClient::instance().destroy_cable(cable, false);
}

void zst_destroy_cable_async(ZstCable * cable)
{
    ZstClient::instance().destroy_cable(cable, true);
}

int zst_graph_recv_tripmeter()
{
	return ZstClient::instance().graph_recv_tripmeter();
}

void zst_reset_graph_recv_tripmeter()
{
	ZstClient::instance().reset_graph_recv_tripmeter();
}

int zst_graph_send_tripmeter()
{
	return ZstClient::instance().graph_send_tripmeter();
}

void zst_reset_graph_send_tripmeter()
{
	return ZstClient::instance().reset_graph_send_tripmeter();
}
