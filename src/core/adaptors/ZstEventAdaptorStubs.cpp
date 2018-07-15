#include <adaptors/ZstEventAdaptor.hpp>
#include <adaptors/ZstComputeAdaptor.hpp>
#include <adaptors/ZstSynchronisableAdaptor.hpp>
#include <adaptors/ZstSessionAdaptor.hpp>
#include <adaptors/ZstHierarchyAdaptor.hpp>
#include <adaptors/ZstSessionAdaptor.hpp>
#include <adaptors/ZstEntityAdaptor.hpp>

#include "ZstTransportAdaptor.hpp"


// --------------------------------------
// Synchronisable adaptor stub functions
// --------------------------------------

void ZstSynchronisableAdaptor::synchronisable_has_event(ZstSynchronisable * synchronisable) {}
void ZstSynchronisableAdaptor::on_synchronisable_activated(ZstSynchronisable * synchronisable) {}
void ZstSynchronisableAdaptor::on_synchronisable_deactivated(ZstSynchronisable * synchronisable) {}
void ZstSynchronisableAdaptor::on_synchronisable_destroyed(ZstSynchronisable * synchronisable) {}


// ---------------
// Entity adaptors
// ---------------
void ZstEntityAdaptor::entity_publish_update(ZstEntityBase * entity) {};


// -----------------------
// Session adaptors
// -----------------------

void ZstSessionAdaptor::on_connected_to_stage() {}
void ZstSessionAdaptor::on_disconnected_from_stage() {};

void ZstSessionAdaptor::on_cable_created(ZstCable * cable) {}
void ZstSessionAdaptor::on_cable_destroyed(ZstCable * cable) {}


// -----------------------
// Hierarchy adaptors
// -----------------------

void ZstHierarchyAdaptor::on_performer_arriving(ZstPerformer * performer) {}
void ZstHierarchyAdaptor::on_performer_leaving(ZstPerformer * performer) {}

void ZstHierarchyAdaptor::on_entity_arriving(ZstEntityBase * entity) {}
void ZstHierarchyAdaptor::on_entity_leaving(ZstEntityBase * entity) {}

void ZstHierarchyAdaptor::on_plug_arriving(ZstPlug * plug) {}
void ZstHierarchyAdaptor::on_plug_leaving(ZstPlug * plug) {}


// -----------------------
// Message adaptors
// -----------------------

void ZstTransportAdaptor::send_message(ZstMsgKind kind, const ZstMsgArgs & args) {}
void ZstTransportAdaptor::send_message(ZstMsgKind kind, const ZstSerialisable & serialisable) {}
void ZstTransportAdaptor::send_message(ZstMsgKind kind, const ZstMsgArgs & args, const ZstSerialisable & serialisable) {}
void ZstTransportAdaptor::send_message(ZstMsgKind kind, const ZstTransportSendType & sendtype, const MessageReceivedAction & action){}
void ZstTransportAdaptor::send_message(ZstMsgKind kind, const ZstTransportSendType & sendtype, const ZstMsgArgs & args, const MessageReceivedAction & action) {}
void ZstTransportAdaptor::send_message(ZstMsgKind kind, const ZstTransportSendType & sendtype, const ZstSerialisable & serialisable, const MessageReceivedAction & action) {}
void ZstTransportAdaptor::send_message(ZstMsgKind kind, const ZstTransportSendType & sendtype, const ZstSerialisable & serialisable, const ZstMsgArgs & args, const MessageReceivedAction & action) {}
void ZstTransportAdaptor::on_receive_msg(ZstMessage * msg) {}


// -----------------------
// Compute adaptors
// -----------------------

void ZstComputeAdaptor::on_compute(ZstComponent * component, ZstInputPlug * plug) {}
