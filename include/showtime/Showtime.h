#pragma once

#include "ZstCore.h"

extern "C" {
	//Init the library
	ZST_CLIENT_EXPORT void zst_init(const char * performer, bool debug);
	ZST_CLIENT_EXPORT void zst_start_file_logging(const char * log_file_path = "");
	ZST_CLIENT_EXPORT void zst_join(const char * stage_address);
    ZST_CLIENT_EXPORT void zst_join_async(const char * stage_address);
    ZST_CLIENT_EXPORT void zst_join_by_name(const char * stage_name);
    ZST_CLIENT_EXPORT void zst_join_by_name_async(const char * stage_name);
    ZST_CLIENT_EXPORT void zst_auto_join();
    ZST_CLIENT_EXPORT void zst_auto_join_by_name(const char * name);
    ZST_CLIENT_EXPORT void zst_auto_join_async();
    ZST_CLIENT_EXPORT void zst_auto_join_by_name_async(const char * name);
    
    ZST_CLIENT_EXPORT void zst_get_discovered_servers(ZstServerBundle & servers);

	//Cleanup
	ZST_CLIENT_EXPORT void zst_destroy();
	ZST_CLIENT_EXPORT void zst_leave();

	//Poll the event queue - for runtimes that have process events from the main thread
	ZST_CLIENT_EXPORT void zst_poll_once();

	//Adaptors
	ZST_CLIENT_EXPORT void zst_add_session_adaptor(ZstSessionAdaptor * adaptor);
	ZST_CLIENT_EXPORT void zst_add_hierarchy_adaptor(ZstHierarchyAdaptor * adaptor);
	ZST_CLIENT_EXPORT void zst_remove_session_adaptor(ZstSessionAdaptor * adaptor);
	ZST_CLIENT_EXPORT void zst_remove_hierarchy_adaptor(ZstHierarchyAdaptor * adaptor);
	
	//Entity activation/deactivation
//    ZST_CLIENT_EXPORT void zst_activate_entity(ZstEntityBase * entity);
//    ZST_CLIENT_EXPORT void zst_activate_entity_async(ZstEntityBase * entity);
	ZST_CLIENT_EXPORT void zst_deactivate_entity(ZstEntityBase * entity);
    ZST_CLIENT_EXPORT void zst_deactivate_entity_async(ZstEntityBase * entity);
	ZST_CLIENT_EXPORT void zst_observe_entity(ZstEntityBase * entity);
	ZST_CLIENT_EXPORT void zst_observe_entity_async(ZstEntityBase * entity);

	//Factories
	ZST_CLIENT_EXPORT ZstEntityBase * zst_create_entity(const ZstURI & creatable_path, const char * name);
	ZST_CLIENT_EXPORT ZstEntityBase * zst_create_entity_async(const ZstURI & creatable_path, const char * name);
	ZST_CLIENT_EXPORT void zst_register_factory(ZstEntityFactory * factory);
	ZST_CLIENT_EXPORT void zst_register_factory_async(ZstEntityFactory * factory);

	//Hierarchy
	ZST_CLIENT_EXPORT ZstPerformer* zst_get_root();
	ZST_CLIENT_EXPORT ZstPerformer* zst_get_performer_by_URI(const ZstURI & path);
	ZST_CLIENT_EXPORT ZstEntityBase* zst_find_entity(const ZstURI & path);
	ZST_CLIENT_EXPORT void zst_get_performers(ZstEntityBundle & bundle);

	//Stage methods
	ZST_CLIENT_EXPORT bool zst_is_connected();
	ZST_CLIENT_EXPORT bool zst_is_connecting();
    ZST_CLIENT_EXPORT bool zst_is_init_completed();
	ZST_CLIENT_EXPORT int zst_ping();

	//Cable management
	ZST_CLIENT_EXPORT ZstCable * zst_connect_cable(ZstInputPlug * input, ZstOutputPlug * output);
    ZST_CLIENT_EXPORT ZstCable * zst_connect_cable_async(ZstInputPlug * input, ZstOutputPlug * output);
	ZST_CLIENT_EXPORT void zst_destroy_cable(ZstCable * cable);
    ZST_CLIENT_EXPORT void zst_destroy_cable_async(ZstCable * cable);
}
