void zst_init(const char * performer);
void zst_join(const char * stage_address);
void zst_destroy();
void zst_leave();
void zst_poll_once();
void zst_attach_connection_event_listener(ZstPerformerEvent * callback);
void zst_attach_performer_event_listener(ZstPerformerEvent * callback, ZstEventAction action);
void zst_attach_component_event_listener(ZstComponentEvent * callback, ZstEventAction action);
void zst_attach_component_type_event_listener(ZstComponentTypeEvent * callback, ZstEventAction action);
void zst_attach_plug_event_listener(ZstPlugEvent * callback, ZstEventAction action);
void zst_attach_cable_event_listener(ZstCableEvent * callback, ZstEventAction action);

void zst_remove_connection_event_listener(ZstPerformerEvent * callback);
void zst_remove_performer_event_listener(ZstPerformerEvent * callback, ZstEventAction action);
void zst_remove_component_event_listener(ZstComponentEvent * callback, ZstEventAction action);
void zst_remove_component_type_event_listener(ZstComponentTypeEvent * callback, ZstEventAction action);
void zst_remove_plug_event_listener(ZstPlugEvent * callback, ZstEventAction action);
void zst_remove_cable_event_listener(ZstCableEvent * callback, ZstEventAction action);

void zst_activate_entity(ZstEntityBase * entity);
void zst_deactivate_entity(ZstEntityBase * entity);

ZstPerformer* zst_get_root();
ZstPerformer* zst_get_performer_by_URI(const ZstURI & path);
ZstEntityBase* zst_find_entity(const ZstURI & path);

bool zst_is_connected();
int zst_ping();

ZstCable * zst_connect_cable(ZstPlug * input, ZstPlug * output);
void zst_destroy_cable(ZstCable * cable);