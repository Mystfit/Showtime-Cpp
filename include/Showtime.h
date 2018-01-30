#pragma once

#include <ZstCore.h>

extern "C" {
	//Init the library
	ZST_CLIENT_EXPORT void zst_init(const char * performer);
	ZST_CLIENT_EXPORT void zst_join(const char * stage_address);

	//Cleanup
	ZST_CLIENT_EXPORT void zst_destroy();
	ZST_CLIENT_EXPORT void zst_leave();

	//Poll the event queue - for runtimes that have process events from the main thread
	ZST_CLIENT_EXPORT void zst_poll_once();

	//Callbacks
	ZST_CLIENT_EXPORT void zst_attach_connection_event_listener(ZstPerformerEvent * callback);
	ZST_CLIENT_EXPORT void zst_attach_performer_event_listener(ZstPerformerEvent * callback, ZstEventAction action);
	ZST_CLIENT_EXPORT void zst_attach_component_event_listener(ZstComponentEvent * callback, ZstEventAction action);
	ZST_CLIENT_EXPORT void zst_attach_component_type_event_listener(ZstComponentTypeEvent * callback, ZstEventAction action);
	ZST_CLIENT_EXPORT void zst_attach_plug_event_listener(ZstPlugEvent * callback, ZstEventAction action);
	ZST_CLIENT_EXPORT void zst_attach_cable_event_listener(ZstCableEvent * callback, ZstEventAction action);

	ZST_CLIENT_EXPORT void zst_remove_connection_event_listener(ZstPerformerEvent * callback);
	ZST_CLIENT_EXPORT void zst_remove_performer_event_listener(ZstPerformerEvent * callback, ZstEventAction action);
	ZST_CLIENT_EXPORT void zst_remove_component_event_listener(ZstComponentEvent * callback, ZstEventAction action);
	ZST_CLIENT_EXPORT void zst_remove_component_type_event_listener(ZstComponentTypeEvent * callback, ZstEventAction action);
	ZST_CLIENT_EXPORT void zst_remove_plug_event_listener(ZstPlugEvent * callback, ZstEventAction action);
	ZST_CLIENT_EXPORT void zst_remove_cable_event_listener(ZstCableEvent * callback, ZstEventAction action);

	//Entity activation/deactivation
	ZST_CLIENT_EXPORT void zst_activate_entity(ZstEntityBase * entity);
	ZST_CLIENT_EXPORT void zst_deactivate_entity(ZstEntityBase * entity);

	//Hierarchy
	ZST_CLIENT_EXPORT ZstPerformer* zst_get_root();
	ZST_CLIENT_EXPORT ZstPerformer* zst_get_performer_by_URI(const ZstURI & path);
	ZST_CLIENT_EXPORT ZstEntityBase* zst_find_entity(const ZstURI & path);

	//Stage methods
	ZST_CLIENT_EXPORT bool zst_is_connected();
	ZST_CLIENT_EXPORT int zst_ping();

	//Cable management
	ZST_CLIENT_EXPORT ZstCable * zst_connect_cable(ZstPlug * input, ZstPlug * output);
	ZST_CLIENT_EXPORT void zst_destroy_cable(ZstCable * cable);

	//Debugging
	ZST_CLIENT_EXPORT int zst_graph_recv_tripmeter();
	ZST_CLIENT_EXPORT void zst_reset_graph_recv_tripmeter();
	ZST_CLIENT_EXPORT int zst_graph_send_tripmeter();
	ZST_CLIENT_EXPORT void zst_reset_graph_send_tripmeter();
}
