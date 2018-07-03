#include <adaptors/ZstEventAdaptor.hpp>
#include <adaptors/ZstPlugAdaptors.hpp>
#include <adaptors/ZstComputeAdaptor.hpp>
#include <adaptors/ZstSynchronisableAdaptor.hpp>
#include <adaptors/ZstSessionAdaptor.hpp>
#include <adaptors/ZstHierarchyAdaptor.hpp>
#include <adaptors/ZstSessionAdaptor.hpp>

#include "ZstStageDispatchAdaptor.hpp"
#include "ZstPerformanceDispatchAdaptor.hpp"

// ----------------------------
// Plug adaptor stub functions
// ----------------------------

void ZstOutputPlugAdaptor::on_plug_fire(ZstOutputPlug * plug) {}


// --------------------------------------
// Synchronisable adaptor stub functions
// --------------------------------------

void ZstSynchronisableAdaptor::synchronisable_has_event(ZstSynchronisable * synchronisable) {}
void ZstSynchronisableAdaptor::on_synchronisable_activated(ZstSynchronisable * synchronisable) {}
void ZstSynchronisableAdaptor::on_synchronisable_deactivated(ZstSynchronisable * synchronisable) {}
void ZstSynchronisableAdaptor::on_synchronisable_destroyed(ZstSynchronisable * synchronisable) {}


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

void ZstStageDispatchAdaptor::send_message(ZstMsgKind kind, bool async, MessageReceivedAction action) {}
void ZstStageDispatchAdaptor::send_message(ZstMsgKind kind, bool async, std::string msg_arg, MessageReceivedAction action) {}
void ZstStageDispatchAdaptor::send_message(ZstMsgKind kind, bool async, const std::vector<std::string> msg_args, MessageReceivedAction action) {}

void ZstStageDispatchAdaptor::send_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable, bool async, MessageReceivedAction action) {}
void ZstStageDispatchAdaptor::send_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable, bool async, std::string msg_arg, MessageReceivedAction action) {}
void ZstStageDispatchAdaptor::send_serialisable_message(ZstMsgKind kind, const ZstSerialisable & serialisable, bool async, const std::vector<std::string> msg_args, MessageReceivedAction action) {}

void ZstStageDispatchAdaptor::send_entity_message(const ZstEntityBase * entity, bool async, MessageReceivedAction action) {}
void ZstStageDispatchAdaptor::on_receive_from_stage(ZstStageMessage * msg) {}

// -----------------------
// Performance message adaptors
// -----------------------

void ZstPerformanceDispatchAdaptor::send_to_performance(ZstOutputPlug * plug) {}
void ZstPerformanceDispatchAdaptor::on_receive_from_performance(ZstPerformanceMessage * msg) {}


// -----------------------
// Compute adaptors
// -----------------------

void ZstComputeAdaptor::on_compute(ZstComponent * component, ZstInputPlug * plug) {}
