#pragma once
#include "../ZstMsgID.h"
#include <schemas/stage_message_generated.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace showtime {

enum ZstTransportRequestBehaviour
{
	PUBLISH = 0,
	SYNC_REPLY,
	ASYNC_REPLY
};


struct ZstMessageReceipt {
	Signal status = Signal_EMPTY;
	ZstTransportRequestBehaviour request_behaviour = ZstTransportRequestBehaviour::PUBLISH;
};


struct ZstTransportException : std::runtime_error {
	using std::runtime_error::runtime_error;
};


typedef std::function<void(ZstMessageReceipt)> ZstMessageReceivedAction;


struct ZstTransportArgs {
	boost::uuids::uuid target_endpoint_UUID = boost::uuids::nil_generator()();
	ZstTransportRequestBehaviour msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
	ZstMsgID msg_ID = 0;
	ZstMessageReceivedAction on_recv_response = [](ZstMessageReceipt receipt) {};
};

}
