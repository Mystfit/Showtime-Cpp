#include <showtime/adaptors/ZstEventAdaptor.hpp>
#include <showtime/adaptors/ZstComputeAdaptor.hpp>
#include <showtime/adaptors/ZstSynchronisableAdaptor.hpp>
#include <showtime/adaptors/ZstSessionAdaptor.hpp>
#include <showtime/adaptors/ZstHierarchyAdaptor.hpp>
#include <showtime/adaptors/ZstLogAdaptor.hpp>
#include <showtime/adaptors/ZstFactoryAdaptor.hpp>
#include <showtime/adaptors/ZstSessionAdaptor.hpp>
#include <showtime/adaptors/ZstEntityAdaptor.hpp>
#include <showtime/adaptors/ZstPluginAdaptor.hpp>
#include <showtime/adaptors/ZstConnectionAdaptor.hpp>
#include <showtime/adaptors/ZstEventAdaptor.hpp>
#include <showtime/ZstSynchronisable.h>
#include <showtime/ZstCable.h>
#include <showtime/entities/ZstPerformer.h>
#include <showtime/entities/ZstPlug.h>
#include <showtime/entities/ZstEntityFactory.h>
#include "ZstServiceDiscoveryAdaptor.hpp"
#include "ZstStageTransportAdaptor.hpp"
#include "ZstGraphTransportAdaptor.hpp"
#include "../ZstHierarchy.h"

namespace showtime {

/*
// --------------------------------------
// Synchronisable adaptor stub functions
// --------------------------------------

void ZstSynchronisableAdaptor::synchronisable_has_event(ZstSynchronisable * synchronisable) {}


// ---------------
// Entity adaptors
// ---------------

void ZstEntityAdaptor::publish_entity_update(ZstEntityBase * entity, const ZstURI& original_path) {}
void ZstEntityAdaptor::request_entity_registration(ZstEntityBase* entity) {}
void ZstEntityAdaptor::request_entity_activation(ZstEntityBase * entity) {}


// ----------------
// Factory adaptors
// ----------------

// -----------------------
// Session adaptors
// -----------------------


ZstCable* ZstSessionAdaptor::connect_cable(ZstInputPlug* input, ZstOutputPlug* output, const ZstTransportRequestBehaviour& sendtype){ return nullptr;}
ZstCableBundle & ZstSessionAdaptor::get_cables(ZstCableBundle & bundle) { return bundle; }
ZstCable * ZstSessionAdaptor::find_cable(const ZstCableAddress & address) { return NULL; }
void ZstSessionAdaptor::destroy_cable(ZstCable* cable){}
void ZstSessionAdaptor::aquire_entity_ownership(ZstEntityBase* entity) {}
void ZstSessionAdaptor::release_entity_ownership(ZstEntityBase* entity) {}
void ZstSessionAdaptor::plug_received_value(ZstInputPlug* plug){}


// -----------------------
// Hierarchy adaptors
// -----------------------

void ZstHierarchyAdaptor::activate_entity(ZstEntityBase* entity, const ZstTransportRequestBehaviour& sendtype){}
void ZstHierarchyAdaptor::deactivate_entity(ZstEntityBase* entity, const ZstTransportRequestBehaviour& sendtype){}
ZstEntityBase* ZstHierarchyAdaptor::find_entity(const ZstURI& path) const { return NULL; }
void ZstHierarchyAdaptor::update_entity_URI(ZstEntityBase* entity, const ZstURI& original_path) {}
ZstPerformer* ZstHierarchyAdaptor::get_local_performer() const { return NULL; }
void ZstHierarchyAdaptor::register_entity_tick(ZstEntityBase* entity) {}
void ZstHierarchyAdaptor::unregister_entity_tick(ZstEntityBase* entity){};

// -----------------------
// Message adaptors
// -----------------------

void ZstTransportAdaptor::connect(const std::string& address) {}
int ZstTransportAdaptor::bind(const std::string& address) { return -1; }
void ZstTransportAdaptor::disconnect() {}
ZstMessageReceipt ZstStageTransportAdaptor::send_msg(Content message_type, flatbuffers::Offset<void> message_content, std::shared_ptr<flatbuffers::FlatBufferBuilder>& buffer_builder, const ZstTransportArgs& args) {
	return ZstMessageReceipt(Signal_OK);
}
ZstMessageReceipt ZstGraphTransportAdaptor::send_msg(flatbuffers::Offset<GraphMessage> message_content, std::shared_ptr<flatbuffers::FlatBufferBuilder>& buffer_builder, const ZstTransportArgs& args) {
	return ZstMessageReceipt(Signal_OK);
}
*/

// -----------------------
// Compute adaptors
// -----------------------
ZstComputeAdaptor::ZstComputeAdaptor(){
	MULTICAST_DELEGATE_INITIALIZER(compute);
}


// -----------------------
// Connection adaptors
// -----------------------

ZstConnectionAdaptor::ZstConnectionAdaptor(){
	MULTICAST_DELEGATE_INITIALIZER(connected_to_server);
	MULTICAST_DELEGATE_INITIALIZER(disconnected_from_server);
	MULTICAST_DELEGATE_INITIALIZER(server_discovered);
	MULTICAST_DELEGATE_INITIALIZER(server_lost);
	MULTICAST_DELEGATE_INITIALIZER(synchronised_graph);
}

ZstEntityAdaptor::ZstEntityAdaptor() {
	MULTICAST_DELEGATE_INITIALIZER(entity_registered);
	MULTICAST_DELEGATE_INITIALIZER(register_entity);
	MULTICAST_DELEGATE_INITIALIZER(disconnect_cable);
	MULTICAST_DELEGATE_INITIALIZER(compute);
	MULTICAST_DELEGATE_INITIALIZER(child_entity_added);
	MULTICAST_DELEGATE_INITIALIZER(child_entity_removed);
}

ZstFactoryAdaptor::ZstFactoryAdaptor() {
	MULTICAST_DELEGATE_INITIALIZER(creatables_updated);
	MULTICAST_DELEGATE_INITIALIZER(entity_created);
}

ZstHierarchyAdaptor::ZstHierarchyAdaptor() {
	MULTICAST_DELEGATE_INITIALIZER(performer_arriving);
	MULTICAST_DELEGATE_INITIALIZER(performer_leaving);
	MULTICAST_DELEGATE_INITIALIZER(entity_arriving);
	MULTICAST_DELEGATE_INITIALIZER(entity_leaving);
	MULTICAST_DELEGATE_INITIALIZER(entity_updated);
	MULTICAST_DELEGATE_INITIALIZER(factory_arriving);
	MULTICAST_DELEGATE_INITIALIZER(factory_leaving);
}

ZstLogAdaptor::ZstLogAdaptor(){
	MULTICAST_DELEGATE_INITIALIZER(formatted_log_record);
}

ZstPluginAdaptor::ZstPluginAdaptor() {
	MULTICAST_DELEGATE_INITIALIZER(plugin_loaded);
	MULTICAST_DELEGATE_INITIALIZER(plugin_unloaded);
}

ZstSessionAdaptor::ZstSessionAdaptor(){
	MULTICAST_DELEGATE_INITIALIZER(cable_created);
	MULTICAST_DELEGATE_INITIALIZER(cable_destroyed);
}

ZstSynchronisableAdaptor::ZstSynchronisableAdaptor() {
	MULTICAST_DELEGATE_INITIALIZER(synchronisable_activated);
	MULTICAST_DELEGATE_INITIALIZER(synchronisable_deactivated);
	MULTICAST_DELEGATE_INITIALIZER(synchronisable_destroyed);
	MULTICAST_DELEGATE_INITIALIZER(synchronisable_updated);
}

ZstServiceDiscoveryAdaptor::ZstServiceDiscoveryAdaptor() {
	MULTICAST_DELEGATE_INITIALIZER(receive_msg);
}

ZstStageTransportAdaptor::ZstStageTransportAdaptor() {
	MULTICAST_DELEGATE_INITIALIZER(receive_msg);
}

ZstGraphTransportAdaptor::ZstGraphTransportAdaptor() {
	MULTICAST_DELEGATE_INITIALIZER(receive_msg);
}

// -----------------------
// Plugin adaptors
// -----------------------


// -------------------
// Log adaptors
// -------------------


}