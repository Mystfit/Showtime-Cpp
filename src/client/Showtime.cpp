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
	ZstClient::instance().join_stage(stage_address, ZstTransportSendType::SYNC_REPLY);
}

void zst_join_async(const char * stage_address){
    ZstClient::instance().join_stage(stage_address, ZstTransportSendType::ASYNC_REPLY);
}

// -----------------
// Cleanup
// -----------------

void zst_destroy() {
	ZstClient::instance().destroy();
}

void zst_leave()
{
    return ZstClient::instance().leave_stage();
}


// -----------------
// Event polling
// -----------------

void zst_poll_once()
{
	ZstClient::instance().process_events();
}

void zst_add_session_adaptor(ZstSessionAdaptor * adaptor)
{
	ZstClient::instance().session()->session_events().add_adaptor(adaptor);
}

void zst_add_hierarchy_adaptor(ZstHierarchyAdaptor * adaptor)
{
	ZstClient::instance().session()->hierarchy()->hierarchy_events().add_adaptor(adaptor);
}

void zst_remove_session_adaptor(ZstSessionAdaptor * adaptor)
{
	ZstClient::instance().session()->session_events().remove_adaptor(adaptor);
}

void zst_remove_hierarchy_adaptor(ZstHierarchyAdaptor * adaptor)
{
	ZstClient::instance().session()->hierarchy()->hierarchy_events().remove_adaptor(adaptor);
}



// ------------------------------
// Entity activation/deactivation
// ------------------------------

void zst_activate_entity(ZstEntityBase * entity)
{
	ZstClient::instance().session()->hierarchy()->activate_entity(entity, ZstTransportSendType::SYNC_REPLY);
}

void zst_activate_entity_async(ZstEntityBase * entity)
{
    ZstClient::instance().session()->hierarchy()->activate_entity(entity, ZstTransportSendType::ASYNC_REPLY);
}

void zst_deactivate_entity(ZstEntityBase * entity)
{
	ZstClient::instance().session()->hierarchy()->destroy_entity(entity, ZstTransportSendType::SYNC_REPLY);
}

void zst_deactivate_entity_async(ZstEntityBase * entity)
{
    ZstClient::instance().session()->hierarchy()->destroy_entity(entity, ZstTransportSendType::ASYNC_REPLY);
}

void zst_observe_entity(ZstEntityBase * entity)
{
	ZstClient::instance().session()->observe_entity(entity, ZstTransportSendType::SYNC_REPLY);
}

void zst_observe_entity_async(ZstEntityBase * entity)
{
	ZstClient::instance().session()->observe_entity(entity, ZstTransportSendType::ASYNC_REPLY);
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

ZstCable * zst_connect_cable(ZstInputPlug * input, ZstOutputPlug * output)
{
	return ZstClient::instance().session()->connect_cable(input, output, ZstTransportSendType::SYNC_REPLY);
}

ZstCable * zst_connect_cable_async(ZstInputPlug * input, ZstOutputPlug * output)
{
    return ZstClient::instance().session()->connect_cable(input, output, ZstTransportSendType::ASYNC_REPLY);
}

void zst_destroy_cable(ZstCable * cable)
{
	ZstClient::instance().session()->destroy_cable(cable, ZstTransportSendType::SYNC_REPLY);
}

void zst_destroy_cable_async(ZstCable * cable)
{
    ZstClient::instance().session()->destroy_cable(cable, ZstTransportSendType::ASYNC_REPLY);
}
