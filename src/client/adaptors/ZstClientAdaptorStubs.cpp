#include <adaptors/ZstSessionAdaptor.hpp>
#include "ZstStageDispatchAdaptor.hpp"
#include "ZstPerformanceDispatchAdaptor.hpp"

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
void ZstStageDispatchAdaptor::on_receive_from_stage(size_t payload_index, ZstMessage * msg) {}


void ZstPerformanceDispatchAdaptor::send_to_performance(ZstOutputPlug * plug) {}
void ZstPerformanceDispatchAdaptor::on_receive_from_performance(ZstMessage * msg) {}
