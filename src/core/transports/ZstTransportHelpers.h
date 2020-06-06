#pragma once
#include <stdexcept>
#include "../ZstMsgID.h"
#include "../ZstMessage.h"
#include "ZstConstants.h"
#include <schemas/stage_message_generated.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>

namespace showtime {
	struct ZstMessageReceipt {
		ZstMessageReceipt() :
			status(Signal_EMPTY),
			request_behaviour(ZstTransportRequestBehaviour::PUBLISH) {}
		ZstMessageReceipt(Signal status, ZstTransportRequestBehaviour request_behaviour = ZstTransportRequestBehaviour::PUBLISH) :
			status(status),
			request_behaviour(request_behaviour) {}

		Signal status;
		ZstTransportRequestBehaviour request_behaviour;
	};


	struct ZstTransportException : std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	struct ZstMessageResponse {
		std::shared_ptr<ZstMessage> response;
		ZstTransportRequestBehaviour send_behaviour;
	};

	typedef std::function<void(const ZstMessageResponse&)> ZstMessageReceivedAction;


	struct ZstTransportArgs {
		boost::uuids::uuid target_endpoint_UUID = boost::uuids::nil_generator()();
		ZstTransportRequestBehaviour msg_send_behaviour = ZstTransportRequestBehaviour::PUBLISH;
		ZstMsgID msg_ID = boost::uuids::nil_uuid();
		ZstMessageReceivedAction on_recv_response = [](ZstMessageResponse response) {};
	};
}
