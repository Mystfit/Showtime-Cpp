#include "Showtime.h"
#include "ZstClient.h"
#include <adaptors/ZstSessionAdaptor.hpp>

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
	ZstClient::instance().join_stage(stage_address, false);
}

void zst_join_async(const char * stage_address){
    ZstClient::instance().join_stage(stage_address, true);
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
	ZstClient::instance().process_events();
}

void add_session_adaptor(ZstSessionAdaptor * adaptor)
{
	ZstClient::instance().session()->add_adaptor(adaptor);
}

void remove_session_adaptor(ZstSessionAdaptor * adaptor)
{
	ZstClient::instance().session()->remove_adaptor(adaptor);
}



// -----------------------
// Entity activation/deactivation
// -----------------------
void zst_activate_entity(ZstEntityBase * entity)
{
	ZstClient::instance().session()->hierarchy()->activate_entity(entity);
}

void zst_activate_entity_async(ZstEntityBase * entity)
{
    ZstClient::instance().session()->hierarchy()->activate_entity(entity, true);
}

void zst_deactivate_entity(ZstEntityBase * entity)
{
	ZstClient::instance().session()->hierarchy()->destroy_entity(entity);
}

void zst_deactivate_entity_async(ZstEntityBase * entity)
{
    ZstClient::instance().session()->hierarchy()->destroy_entity(entity, true);
}

void zst_deactivate_plug(ZstPlug * plug)
{
	ZstClient::instance().session()->hierarchy()->destroy_plug(plug, false);
}

void zst_deactivate_plug_async(ZstPlug * plug)
{
	ZstClient::instance().session()->hierarchy()->destroy_plug(plug, true);
}


// -------------
// Hierarchy
// -------------

ZstPerformer * zst_get_root()
{
	return ZstClient::instance().session()->hierarchy()->get_local_performer();
}

ZstPerformer * zst_get_performer_by_URI(const ZstURI & path)
{
    return ZstClient::instance().session()->hierarchy()->get_performer_by_URI(path);
}

ZstEntityBase* zst_find_entity(const ZstURI & path)
{
	return ZstClient::instance().session()->hierarchy()->find_entity(path);
}


// -------------
// Stage status
// -------------

bool zst_is_connected()
{
	return ZstClient::instance().is_connected_to_stage();
}

bool zst_is_connecting()
{
	return ZstClient::instance().is_connecting_to_stage();
}

bool zst_is_init_completed()
{
    return ZstClient::instance().is_init_complete();
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
	return ZstClient::instance().session()->connect_cable(input, output, false);
}

ZstCable * zst_connect_cable_async(ZstPlug * input, ZstPlug * output)
{
    return ZstClient::instance().session()->connect_cable(input, output, true);
}

void zst_destroy_cable(ZstCable * cable)
{
	ZstClient::instance().session()->destroy_cable(cable, false);
}

void zst_destroy_cable_async(ZstCable * cable)
{
    ZstClient::instance().session()->destroy_cable(cable, true);
}
