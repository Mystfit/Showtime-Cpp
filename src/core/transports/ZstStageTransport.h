#pragma once

#include "ZstExports.h"
#include "ZstTransportLayerBase.hpp"
#include "../ZstStageMessage.h"
#include "../adaptors/ZstStageTransportAdaptor.hpp"

namespace showtime 
{
    class ZST_CLASS_EXPORTED ZstStageTransport : 
		public ZstTransportLayerBase<ZstStageMessage, ZstStageTransportAdaptor>,
		public ZstStageTransportAdaptor
	{
	public:
		ZST_EXPORT virtual ZstMessageReceipt send_msg(Content message_type, flatbuffers::Offset<void> message_content, flatbuffers::FlatBufferBuilder& buffer_builder, const ZstTransportArgs& args) override;
	};
}
