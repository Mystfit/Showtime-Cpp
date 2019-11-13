#include "ZstStageTransport.h"
namespace showtime
{
	ZstMessageReceipt ZstStageTransport::send_msg(Content message_type, flatbuffers::Offset<void> message_content, flatbuffers::FlatBufferBuilder& buffer_builder, const ZstTransportArgs& args)
	{
		// Make a copy of the transport args so we can generate a new message ID if required
		auto copy_args = args;
		copy_args.msg_ID = (args.msg_ID > 0) ? args.msg_ID : ZstMsgIDManager::next_id();

		// Create the stage message
		auto stage_msg = CreateStageMessage(buffer_builder, message_type, message_content, copy_args.msg_ID);
		FinishStageMessageBuffer(buffer_builder, stage_msg);

		begin_send_message(buffer_builder.GetBufferPointer(), buffer_builder.GetSize(), args);
		return ZstMessageReceipt{ Signal_OK };
	}
}
