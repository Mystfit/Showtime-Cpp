#include <adaptors/ZstSessionAdaptor.hpp>
#include "ZstMessageAdaptor.hpp"


// -----------------------
// Message adaptors
// -----------------------

ZstMessageReceipt ZstMessageAdaptor::on_send_to_stage(ZstStageMessage * msg, bool async, MessageBoundAction action) { return ZstMessageReceipt{ ZstMsgKind::EMPTY, false}; };
void ZstMessageAdaptor::on_receive_from_stage(int payload_index, ZstStageMessage * msg) {};
void ZstMessageAdaptor::on_process_stage_response(ZstStageMessage * msg) {};
void ZstMessageAdaptor::on_send_to_performance(ZstPerformanceMessage * msg) {}
void ZstMessageAdaptor::on_receive_from_performance(ZstPerformanceMessage * msg) {}


// -----------------------
// Session adaptors
// -----------------------

void ZstSessionAdaptor::on_connected_to_stage() {}
void ZstSessionAdaptor::on_disconnected_from_stage() {};

void ZstSessionAdaptor::on_performer_arriving(ZstPerformer * performer) {};
void ZstSessionAdaptor::on_performer_leaving(ZstPerformer * performer) {};

void ZstSessionAdaptor::on_entity_arriving(ZstEntityBase * entity) {};
void ZstSessionAdaptor::on_entity_leaving(ZstEntityBase * entity) {};

void ZstSessionAdaptor::on_plug_arriving(ZstPlug * plug) {};
void ZstSessionAdaptor::on_plug_leaving(ZstPlug * plug) {};
void ZstSessionAdaptor::on_plug_received_value(ZstInputPlug * plug) {};

void ZstSessionAdaptor::on_cable_created(ZstCable * cable) {};
void ZstSessionAdaptor::on_cable_destroyed(ZstCable * cable) {};
