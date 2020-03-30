#include "ZstStageTransport.h"
namespace showtime
{
	ZstMessageReceipt ZstStageTransport::send_msg(Content message_type, flatbuffers::Offset<void> message_content, flatbuffers::FlatBufferBuilder& buffer_builder, const ZstTransportArgs& args)
	{
		// Make a copy of the transport args so we can generate a new message ID if required
		auto copy_args = args;
        copy_args.msg_ID = (args.msg_ID.is_nil()) ? ZstMsgIDManager::next_id() : args.msg_ID ;

		//ZstLog::net(LogLevel::debug, "Sending stage message with ID: {}", copy_args.msg_ID);

		// Create the stage message
        auto msg_id_data = buffer_builder.CreateVector(std::vector<uint8_t>(copy_args.msg_ID.begin(), copy_args.msg_ID.end()));
        
		auto stage_msg = CreateStageMessage(buffer_builder, message_type, message_content, msg_id_data);
		FinishStageMessageBuffer(buffer_builder, stage_msg);

		begin_send_message(buffer_builder.GetBufferPointer(), buffer_builder.GetSize(), copy_args);
		return ZstMessageReceipt(Signal_OK);
	}
}
