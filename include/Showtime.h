#pragma once

#include <ZstCore.h>

extern "C" {
	//Init the library
	ZST_CLIENT_EXPORT void zst_init(const char * performer, bool debug);
	ZST_CLIENT_EXPORT void zst_start_file_logging(const char * log_file_path = "");
	ZST_CLIENT_EXPORT void zst_join(const char * stage_address);
    ZST_CLIENT_EXPORT void zst_join_async(const char * stage_address);

	//Cleanup
	ZST_CLIENT_EXPORT void zst_destroy();
	ZST_CLIENT_EXPORT void zst_leave();
    ZST_CLIENT_EXPORT void zst_leave_immediately();

	//Poll the event queue - for runtimes that have process events from the main thread
	ZST_CLIENT_EXPORT void zst_poll_once();

	//Adaptors
	ZST_CLIENT_EXPORT void zst_add_session_adaptor(ZstSessionAdaptor * adaptor);
	ZST_CLIENT_EXPORT void zst_add_hierarchy_adaptor(ZstHierarchyAdaptor * adaptor);
	ZST_CLIENT_EXPORT void zst_remove_session_adaptor(ZstSessionAdaptor * adaptor);
	ZST_CLIENT_EXPORT void zst_remove_hierarchy_adaptor(ZstHierarchyAdaptor * adaptor);


	//Entity activation/deactivation
	ZST_CLIENT_EXPORT void zst_activate_entity(ZstEntityBase * entity);
    ZST_CLIENT_EXPORT void zst_activate_entity_async(ZstEntityBase * entity);
	ZST_CLIENT_EXPORT void zst_deactivate_entity(ZstEntityBase * entity);
    ZST_CLIENT_EXPORT void zst_deactivate_entity_async(ZstEntityBase * entity);
	ZST_CLIENT_EXPORT void zst_deactivate_plug(ZstPlug * plug);
	ZST_CLIENT_EXPORT void zst_deactivate_plug_async(ZstPlug * plug);

	//Hierarchy
	ZST_CLIENT_EXPORT ZstPerformer* zst_get_root();
	ZST_CLIENT_EXPORT ZstPerformer* zst_get_performer_by_URI(const ZstURI & path);
	ZST_CLIENT_EXPORT ZstEntityBase* zst_find_entity(const ZstURI & path);

	//Stage methods
	ZST_CLIENT_EXPORT bool zst_is_connected();
	ZST_CLIENT_EXPORT bool zst_is_connecting();
    ZST_CLIENT_EXPORT bool zst_is_init_completed();
	ZST_CLIENT_EXPORT int zst_ping();

	//Cable management
	ZST_CLIENT_EXPORT ZstCable * zst_connect_cable(ZstPlug * input, ZstPlug * output);
    ZST_CLIENT_EXPORT ZstCable * zst_connect_cable_async(ZstPlug * input, ZstPlug * output);
	ZST_CLIENT_EXPORT void zst_destroy_cable(ZstCable * cable);
    ZST_CLIENT_EXPORT void zst_destroy_cable_async(ZstCable * cable);
}
