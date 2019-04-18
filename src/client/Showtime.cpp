#include "Showtime.h"
#include "ZstClient.h"
#include "ZstClientSession.h"
#include "adaptors/ZstSessionAdaptor.hpp"

using namespace std;

inline bool LIBRARY_INIT_GUARD() { 
	if (!ZstClient::instance().is_init_complete()) {
		ZstLog::net(LogLevel::error, "Showtime library has not been initialised."); 
		return false;
	} 
	return true;
}

inline bool LIBRARY_CONNECTED_GUARD() {
	if (!LIBRARY_INIT_GUARD()) {
		return false;
	}

	if (!ZstClient::instance().is_connected_to_stage()) {
		ZstLog::net(LogLevel::warn, "Not connected to a Showtime stage.");
		return false;
	}
	return true;
}


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
	if(LIBRARY_INIT_GUARD()) ZstClient::instance().join_stage(stage_address, ZstTransportSendType::SYNC_REPLY);
}

void zst_join_async(const char * stage_address){
	if (LIBRARY_INIT_GUARD()) ZstClient::instance().join_stage(stage_address, ZstTransportSendType::ASYNC_REPLY);
}

// -----------------
// Cleanup
// -----------------

void zst_destroy()
{
	if (LIBRARY_INIT_GUARD()) ZstClient::instance().destroy();
}

void zst_leave()
{
	if (LIBRARY_INIT_GUARD()) ZstClient::instance().leave_stage();
}


// -----------------
// Event polling
// -----------------

void zst_poll_once()
{
	if (LIBRARY_INIT_GUARD()) ZstClient::instance().process_events();
}

void zst_add_session_adaptor(ZstSessionAdaptor * adaptor)
{
	if (LIBRARY_INIT_GUARD()) ZstClient::instance().session()->session_events().add_adaptor(adaptor);
}

void zst_add_hierarchy_adaptor(ZstHierarchyAdaptor * adaptor)
{
	if (LIBRARY_INIT_GUARD()) ZstClient::instance().session()->hierarchy()->hierarchy_events().add_adaptor(adaptor);
}

void zst_remove_session_adaptor(ZstSessionAdaptor * adaptor)
{
	if (LIBRARY_INIT_GUARD()) ZstClient::instance().session()->session_events().remove_adaptor(adaptor);
}

void zst_remove_hierarchy_adaptor(ZstHierarchyAdaptor * adaptor)
{
	if (LIBRARY_INIT_GUARD()) ZstClient::instance().session()->hierarchy()->hierarchy_events().remove_adaptor(adaptor);
}



// ------------------------------
// Entity activation/deactivation
// ------------------------------

//void zst_activate_entity(ZstEntityBase * entity)
//{
//    if (LIBRARY_CONNECTED_GUARD()) ZstClient::instance().session()->hierarchy()->activate_entity(entity, ZstTransportSendType::SYNC_REPLY);
//}

void zst_activate_entity_async(ZstEntityBase * entity)
{
	if (LIBRARY_CONNECTED_GUARD()) ZstClient::instance().session()->hierarchy()->activate_entity(entity, ZstTransportSendType::ASYNC_REPLY);
}

void zst_deactivate_entity(ZstEntityBase * entity)
{
	if (LIBRARY_CONNECTED_GUARD()) ZstClient::instance().session()->hierarchy()->destroy_entity(entity, ZstTransportSendType::SYNC_REPLY);
}

void zst_deactivate_entity_async(ZstEntityBase * entity)
{
	if (LIBRARY_CONNECTED_GUARD()) ZstClient::instance().session()->hierarchy()->destroy_entity(entity, ZstTransportSendType::ASYNC_REPLY);
}

void zst_observe_entity(ZstEntityBase * entity)
{
	if (LIBRARY_CONNECTED_GUARD()) ZstClient::instance().session()->observe_entity(entity, ZstTransportSendType::SYNC_REPLY);
}

void zst_observe_entity_async(ZstEntityBase * entity)
{
	if (LIBRARY_CONNECTED_GUARD()) ZstClient::instance().session()->observe_entity(entity, ZstTransportSendType::ASYNC_REPLY);
}


// ---------
// Factories
// ---------

ZstEntityBase * zst_create_entity(const ZstURI & creatable_path, const char * name)
{
	return ZstClient::instance().session()->hierarchy()->create_entity(creatable_path, name, ZstTransportSendType::SYNC_REPLY);
}

ZstEntityBase * zst_create_entity_async(const ZstURI & creatable_path, const char * name)
{
	return ZstClient::instance().session()->hierarchy()->create_entity(creatable_path, name, ZstTransportSendType::ASYNC_REPLY);
}

void zst_register_factory(ZstEntityFactory * factory)
{
	if (!LIBRARY_INIT_GUARD()) return;

	//Add the factory to the root performer first to allow for offline factory registration
	zst_get_root()->add_child(factory);

	if (LIBRARY_CONNECTED_GUARD()) ZstClient::instance().session()->hierarchy()->activate_entity(factory, ZstTransportSendType::SYNC_REPLY);
}

void zst_register_factory_async(ZstEntityFactory * factory)
{
	if (!LIBRARY_INIT_GUARD()) return;

	//Add the factory to the root performer first to allow for offline factory registration
	zst_get_root()->add_child(factory);

	if (LIBRARY_CONNECTED_GUARD()) ZstClient::instance().session()->hierarchy()->activate_entity(factory, ZstTransportSendType::ASYNC_REPLY);
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
	if (!LIBRARY_INIT_GUARD()) return NULL;
    return ZstClient::instance().session()->hierarchy()->get_performer_by_URI(path);
}

ZstEntityBase* zst_find_entity(const ZstURI & path)
{
	if (!LIBRARY_INIT_GUARD()) return NULL;
	return ZstClient::instance().session()->hierarchy()->find_entity(path);
}

void zst_get_performers(ZstEntityBundle & bundle)
{
	if (!LIBRARY_INIT_GUARD()) return;
	ZstClient::instance().session()->hierarchy()->get_performers(bundle);
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
	if (!LIBRARY_CONNECTED_GUARD()) return NULL;
	return ZstClient::instance().session()->connect_cable(input, output, ZstTransportSendType::SYNC_REPLY);
}

ZstCable * zst_connect_cable_async(ZstInputPlug * input, ZstOutputPlug * output)
{
	if (!LIBRARY_CONNECTED_GUARD()) return NULL;
	return ZstClient::instance().session()->connect_cable(input, output, ZstTransportSendType::ASYNC_REPLY);
}

void zst_destroy_cable(ZstCable * cable)
{
	if (LIBRARY_CONNECTED_GUARD()) ZstClient::instance().session()->destroy_cable(cable, ZstTransportSendType::SYNC_REPLY);
}

void zst_destroy_cable_async(ZstCable * cable)
{
	if (LIBRARY_CONNECTED_GUARD()) ZstClient::instance().session()->destroy_cable(cable, ZstTransportSendType::ASYNC_REPLY);
}
