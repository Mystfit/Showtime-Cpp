#include "ZstStageTransport.h"
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

namespace showtime
{
	ZstMessageReceipt ZstStageTransport::send_msg(Content message_type, flatbuffers::Offset<void> message_content, std::shared_ptr<flatbuffers::FlatBufferBuilder>& buffer_builder, const ZstTransportArgs& args)
	{
		// Make a copy of the transport args so we can generate a new message ID if required
		auto copy_args = args;
        copy_args.msg_ID = (args.msg_ID.is_nil()) ? ZstMsgIDManager::next_id() : args.msg_ID ;

		// Create the stage message
		auto stage_msg = CreateStageMessage(*buffer_builder, message_type, message_content);
		FinishStageMessageBuffer(*buffer_builder, stage_msg);

		auto verifier = flatbuffers::Verifier(buffer_builder->GetBufferPointer(), buffer_builder->GetSize());
		if (!VerifyStageMessageBuffer(verifier))
			throw;

		begin_send_message(buffer_builder, copy_args);
		return ZstMessageReceipt(Signal_OK);
	}

	Signal ZstStageTransport::get_signal(const std::shared_ptr<ZstMessage>& msg)
	{
		return ZstStageTransport::get_signal(std::dynamic_pointer_cast<ZstStageMessage>(msg));
	}

	Signal ZstStageTransport::get_signal(const std::shared_ptr<ZstStageMessage>& msg)
	{
		if (!msg) {
			Log::net(Log::Level::error, "Message is NULL");
			return Signal_EMPTY;
		}
		if (msg->buffer()->content_type() == Content_SignalMessage) {
			return msg->buffer()->content_as_SignalMessage()->signal();
		}
		Log::net(Log::Level::error, "Can't get signal from message. Type is {}", EnumNameContent(msg->buffer()->content_type()));
		return Signal_EMPTY;
	}

	bool ZstStageTransport::verify_signal(const std::shared_ptr<ZstMessage>& msg, Signal expected, const std::string& error_prefix)
	{
		return verify_signal(std::dynamic_pointer_cast<ZstStageMessage>(msg), expected, error_prefix);
	}

	bool ZstStageTransport::verify_signal(const std::shared_ptr<ZstStageMessage>& msg, Signal expected, const std::string& error_prefix)
	{
		if (!msg) {
			Log::net(Log::Level::error, "{} timed out", error_prefix);
			return false;
		}
		auto signal = ZstStageTransport::get_signal(msg);
		if (signal != expected) {
			Log::net(Log::Level::error, "{} failed with status {}", error_prefix, EnumNameSignal(signal));
			return false;
		}
		return true;
	}
}
