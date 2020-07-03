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


// --------------------------------------
// Synchronisable adaptor stub functions
// --------------------------------------


void ZstSynchronisableAdaptor::synchronisable_has_event(ZstSynchronisable * synchronisable) {}
//void ZstSynchronisableAdaptor::on_synchronisable_activated(ZstSynchronisable * synchronisable) {}
//void ZstSynchronisableAdaptor::on_synchronisable_deactivated(ZstSynchronisable * synchronisable) {}
//void ZstSynchronisableAdaptor::on_synchronisable_destroyed(ZstSynchronisable * synchronisable, bool already_removed) {}
//void ZstSynchronisableAdaptor::on_synchronisable_updated(ZstSynchronisable * synchronisable) {}


// ---------------
// Entity adaptors
// ---------------

//void ZstEntityAdaptor::on_entity_registered(ZstEntityBase* entity){}
void ZstEntityAdaptor::publish_entity_update(ZstEntityBase * entity, const ZstURI& original_path) {}
//void ZstEntityAdaptor::on_register_entity(ZstEntityBase * entity){}
void ZstEntityAdaptor::request_entity_registration(ZstEntityBase* entity) {}
void ZstEntityAdaptor::request_entity_activation(ZstEntityBase * entity) {}
//void ZstEntityAdaptor::on_disconnect_cable(const ZstCableAddress& cable) {}


// ----------------
// Factory adaptors
// ----------------

//void ZstFactoryAdaptor::on_creatables_updated(ZstEntityFactory * factory) {}
//void ZstFactoryAdaptor::on_entity_created(ZstEntityBase * entity) {}


// -----------------------
// Session adaptors
// -----------------------

//void ZstSessionAdaptor::on_cable_created(ZstCable * cable) {}
//void ZstSessionAdaptor::on_cable_destroyed(const ZstCableAddress& cable) {}

ZstCableBundle & ZstSessionAdaptor::get_cables(ZstCableBundle & bundle) { return bundle; }
ZstCable * ZstSessionAdaptor::find_cable(const ZstCableAddress & address) { return NULL; }
void ZstSessionAdaptor::destroy_cable(ZstCable* cable){}
void ZstSessionAdaptor::aquire_entity_ownership(ZstEntityBase* entity) {}
void ZstSessionAdaptor::release_entity_ownership(ZstEntityBase* entity) {}
void ZstSessionAdaptor::plug_received_value(ZstInputPlug* plug){}


// -----------------------
// Hierarchy adaptors
// -----------------------

//void ZstHierarchyAdaptor::on_performer_arriving(ZstPerformer * performer) {}
//void ZstHierarchyAdaptor::on_performer_leaving(const ZstURI& performer_path) {}
//void ZstHierarchyAdaptor::on_entity_arriving(ZstEntityBase * entity) {}
//void ZstHierarchyAdaptor::on_entity_leaving(const ZstURI& entity_path) {}
//void ZstHierarchyAdaptor::on_entity_updated(ZstEntityBase* entity){}
//void ZstHierarchyAdaptor::on_factory_arriving(ZstEntityFactory * factory) {}
//void ZstHierarchyAdaptor::on_factory_leaving(const ZstURI& factory_path) {}

void ZstHierarchyAdaptor::activate_entity(ZstEntityBase* entity, const ZstTransportRequestBehaviour& sendtype){}
void ZstHierarchyAdaptor::deactivate_entity(ZstEntityBase* entity, const ZstTransportRequestBehaviour& sendtype){}
ZstEntityBase* ZstHierarchyAdaptor::find_entity(const ZstURI& path) const { return NULL; }
void ZstHierarchyAdaptor::update_entity_URI(ZstEntityBase* entity, const ZstURI& original_path) {}
ZstPerformer* ZstHierarchyAdaptor::get_local_performer() const { return NULL; }



// -----------------------
// Message adaptors
// -----------------------

//void ZstStageTransportAdaptor::on_receive_msg(const std::shared_ptr<ZstStageMessage>& msg) {};
//void ZstGraphTransportAdaptor::on_receive_msg(const std::shared_ptr<ZstPerformanceMessage>& msg) {};
//void ZstServiceDiscoveryAdaptor::on_receive_msg(const std::shared_ptr<ZstServerBeaconMessage>& msg) {};

void ZstTransportAdaptor::connect(const std::string& address) {}
int ZstTransportAdaptor::bind(const std::string& address) { return -1; }
void ZstTransportAdaptor::disconnect() {}
ZstMessageReceipt ZstStageTransportAdaptor::send_msg(Content message_type, flatbuffers::Offset<void> message_content, std::shared_ptr<flatbuffers::FlatBufferBuilder>& buffer_builder, const ZstTransportArgs& args) {
	return ZstMessageReceipt(Signal_OK);
}
ZstMessageReceipt ZstGraphTransportAdaptor::send_msg(flatbuffers::Offset<GraphMessage> message_content, std::shared_ptr<flatbuffers::FlatBufferBuilder>& buffer_builder, const ZstTransportArgs& args) {
	return ZstMessageReceipt(Signal_OK);
}


// -----------------------
// Compute adaptors
// -----------------------

//void ZstComputeAdaptor::on_compute(ZstComponent * component, ZstInputPlug * plug) {}



// -----------------------
// Plugin adaptors
// -----------------------
//
//void ZstPluginAdaptor::on_plugin_loaded(std::shared_ptr<ZstPlugin> plugin) {}
//void ZstPluginAdaptor::on_plugin_unloaded(std::shared_ptr<ZstPlugin> plugin) {}

// -------------------
// Log adaptors
// -------------------
//void ZstLogAdaptor::on_log_record(const Log::Record& record) {}

}