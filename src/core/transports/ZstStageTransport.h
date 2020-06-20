#pragma once

#include <showtime/ZstExports.h>
#include "ZstTransportLayerBase.hpp"
#include "../ZstStageMessage.h"
#include "../adaptors/ZstStageTransportAdaptor.hpp"

namespace showtime 
{
    class ZST_CLASS_EXPORTED ZstStageTransport : 
		public ZstTransportLayer<ZstStageMessage, ZstStageTransportAdaptor>,
		public ZstStageTransportAdaptor
	{
	public:
		ZST_EXPORT virtual ZstMessageReceipt send_msg(Content message_type, flatbuffers::Offset<void> message_content, std::shared_ptr<flatbuffers::FlatBufferBuilder>& buffer_builder, const ZstTransportArgs& args) override;

		ZST_EXPORT static Signal get_signal(const std::shared_ptr<ZstMessage>& msg);
		ZST_EXPORT static Signal get_signal(const std::shared_ptr<ZstStageMessage>& msg);

		ZST_EXPORT static bool verify_signal(const std::shared_ptr<ZstMessage>& msg, Signal expected, const std::string& error_prefix);
		ZST_EXPORT static bool verify_signal(const std::shared_ptr<ZstStageMessage>& msg, Signal expected, const std::string& error_prefix);
	};
}
