#include "adaptors/ZstEventAdaptor.hpp"
#include "adaptors/ZstComputeAdaptor.hpp"
#include "adaptors/ZstSynchronisableAdaptor.hpp"
#include "adaptors/ZstSessionAdaptor.hpp"
#include "adaptors/ZstHierarchyAdaptor.hpp"
#include "adaptors/ZstFactoryAdaptor.hpp"
#include "adaptors/ZstSessionAdaptor.hpp"
#include "adaptors/ZstEntityAdaptor.hpp"
#include "adaptors/ZstEventAdaptor.hpp"

#include "ZstServiceDiscoveryAdaptor.hpp"
#include "ZstStageTransportAdaptor.hpp"
#include "ZstGraphTransportAdaptor.hpp"

#include "../ZstHierarchy.h"

#include "ZstSynchronisable.h"
#include "ZstCable.h"
#include "entities/ZstPerformer.h"
#include "entities/ZstPlug.h"
#include "entities/ZstEntityFactory.h"

namespace showtime {


// --------------------------------------
// Synchronisable adaptor stub functions
// --------------------------------------


void ZstSynchronisableAdaptor::on_synchronisable_has_event(ZstSynchronisable * synchronisable) {}
void ZstSynchronisableAdaptor::on_synchronisable_activated(ZstSynchronisable * synchronisable) {}
void ZstSynchronisableAdaptor::on_synchronisable_deactivated(ZstSynchronisable * synchronisable) {}
void ZstSynchronisableAdaptor::on_synchronisable_destroyed(ZstSynchronisable * synchronisable, bool already_removed) {}
void ZstSynchronisableAdaptor::on_synchronisable_updated(ZstSynchronisable * synchronisable) {}


// ---------------
// Entity adaptors
// ---------------

void ZstEntityAdaptor::on_entity_registered(ZstEntityBase* entity){}
void ZstEntityAdaptor::publish_entity_update(ZstEntityBase * entity, const ZstURI& original_path) {}
void ZstEntityAdaptor::on_register_entity(ZstEntityBase * entity){}
void ZstEntityAdaptor::on_request_entity_registration(ZstEntityBase* entity) {}
void ZstEntityAdaptor::on_request_entity_activation(ZstEntityBase * entity) {}
void ZstEntityAdaptor::on_disconnect_cable(ZstCable * cable) {}


// ----------------
// Factory adaptors
// ----------------

void ZstFactoryAdaptor::on_creatables_updated(ZstEntityFactory * factory) {}
void ZstFactoryAdaptor::on_entity_created(ZstEntityBase * entity) {}


// -----------------------
// Session adaptors
// -----------------------

void ZstSessionAdaptor::on_cable_created(ZstCable * cable) {}
void ZstSessionAdaptor::on_cable_destroyed(ZstCable * cable) {}

ZstCableBundle & ZstSessionAdaptor::get_cables(ZstCableBundle & bundle) { return bundle; }
ZstCable * ZstSessionAdaptor::find_cable(const ZstCableAddress & address) { return NULL; }
void ZstSessionAdaptor::destroy_cable(ZstCable* cable){}
void ZstSessionAdaptor::aquire_entity_ownership(ZstEntityBase* entity) {}
void ZstSessionAdaptor::release_entity_ownership(ZstEntityBase* entity) {}


// -----------------------
// Hierarchy adaptors
// -----------------------

void ZstHierarchyAdaptor::on_performer_arriving(ZstPerformer * performer) {}
void ZstHierarchyAdaptor::on_performer_leaving(ZstPerformer * performer) {}
void ZstHierarchyAdaptor::on_entity_arriving(ZstEntityBase * entity) {}
void ZstHierarchyAdaptor::on_entity_leaving(ZstEntityBase * entity) {}
void ZstHierarchyAdaptor::on_entity_updated(ZstEntityBase* entity){}
void ZstHierarchyAdaptor::on_plug_arriving(ZstPlug * plug) {}
void ZstHierarchyAdaptor::on_plug_leaving(ZstPlug * plug) {}
void ZstHierarchyAdaptor::on_factory_arriving(ZstEntityFactory * factory) {}
void ZstHierarchyAdaptor::on_factory_leaving(ZstEntityFactory * factory) {}

void ZstHierarchyAdaptor::activate_entity(ZstEntityBase* entity, const ZstTransportRequestBehaviour& sendtype){}
void ZstHierarchyAdaptor::deactivate_entity(ZstEntityBase* entity, const ZstTransportRequestBehaviour& sendtype){}
ZstEntityBase* ZstHierarchyAdaptor::find_entity(const ZstURI& path) const { return NULL; }
void ZstHierarchyAdaptor::update_entity_URI(ZstEntityBase* entity, const ZstURI& original_path) {}
ZstPerformer* ZstHierarchyAdaptor::get_local_performer() const { return NULL; }



// -----------------------
// Message adaptors
// -----------------------

void ZstStageTransportAdaptor::on_receive_msg(std::shared_ptr<ZstStageMessage> msg) {};
void ZstGraphTransportAdaptor::on_receive_msg(std::shared_ptr<ZstPerformanceMessage> msg) {};
void ZstServiceDiscoveryAdaptor::on_receive_msg(std::shared_ptr<ZstServerBeaconMessage> msg) {};

void ZstTransportAdaptor::connect(const std::string& address) {}
int ZstTransportAdaptor::bind(const std::string& address) { return -1; }
void ZstTransportAdaptor::disconnect() {}
ZstMessageReceipt ZstStageTransportAdaptor::send_msg(Content message_type, flatbuffers::Offset<void> message_content, flatbuffers::FlatBufferBuilder& buffer_builder, const ZstTransportArgs& args) {
	return ZstMessageReceipt(Signal_OK);
}
ZstMessageReceipt ZstGraphTransportAdaptor::send_msg(flatbuffers::Offset<GraphMessage> message_content, flatbuffers::FlatBufferBuilder& buffer_builder, const ZstTransportArgs& args) {
	return ZstMessageReceipt(Signal_OK);
}


// -----------------------
// Compute adaptors
// -----------------------

void ZstComputeAdaptor::on_compute(ZstComponent * component, ZstInputPlug * plug) {}

}
