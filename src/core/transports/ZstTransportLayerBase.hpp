#pragma once

#include <string>
#include <functional>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>

#include "ZstExports.h"
#include "ZstTransportHelpers.h"


#include "../ZstEventDispatcher.hpp"
#include "../ZstActor.h"
#include "../ZstStageMessage.h"
#include "../ZstPerformanceMessage.h"
#include "../adaptors/ZstStageTransportAdaptor.hpp"
#include "../adaptors/ZstGraphTransportAdaptor.hpp"
#include "../ZstMessageSupervisor.hpp"

namespace showtime {

//Forwards
class ZstTransportAdaptor;


template<typename Message_T, typename Adaptor_T>
class ZST_CLASS_EXPORTED ZstTransportLayerBase : 
	public ZstMessageSupervisor
{
public:
	ZST_EXPORT ZstTransportLayerBase() :
		ZstMessageSupervisor(std::make_shared<cf::time_watcher>(), STAGE_TIMEOUT),
		m_is_active(false),
		m_dispatch_events(std::make_shared<ZstEventDispatcher<std::shared_ptr<Adaptor_T> > >("transport events")){
	}
	ZST_EXPORT virtual ~ZstTransportLayerBase() {}

	ZST_EXPORT virtual void init() {
		m_is_active = true;
	}

	ZST_EXPORT virtual void destroy() {}

	ZST_EXPORT virtual void process_events(){
		m_dispatch_events->process_events();
	}

	ZST_EXPORT virtual void receive_msg(Message_T* msg){
		m_dispatch_events->defer([msg](std::shared_ptr<Adaptor_T> adaptor) {
			adaptor->on_receive_msg(msg);
			}, [this, msg](ZstEventStatus status) {
				process_response(msg->id(), ZstMessageReceipt{ Signal::Signal_OK });
			});
	}

	ZST_EXPORT virtual void receive_msg(Message_T* msg, ZstEventCallback on_complete) {
		m_dispatch_events->defer([msg](std::shared_ptr<Adaptor_T> adaptor) {
			adaptor->on_receive_msg(msg);
			}, [this, msg, on_complete](ZstEventStatus status) {
				process_response(msg->id(), ZstMessageReceipt{ Signal::Signal_OK });
				on_complete(status);
			});
	}

	//Message sending implementation for the transport
	ZST_EXPORT virtual void send_message_impl(uint8_t * msg_buffer, const ZstTransportArgs & args) = 0;

	//Message supervision
	ZST_EXPORT virtual ZstMessageReceipt send_sync_message(ZstMessage* msg, const ZstTransportArgs& args){
		auto future = register_response(msg->id());
		ZstMessageReceipt msg_response{ ZstMsgKind::EMPTY };

		auto a = flatbuffers::FlatBufferBuilder();
		auto b = a.GetBufferPointer();

		send_message_impl(msg, args);
		try {
			//Blocking call on get
			msg_response = future.get();
		}
		catch (const ZstTimeoutException & e) {
			ZstLog::net(LogLevel::error, "Server response timed out", e.what());
			msg_response.status = ZstMsgKind::ERR_STAGE_TIMEOUT;
		}

		enqueue_resolved_promise(msg->id());
		msg_response.request_behaviour = ZstTransportRequestBehaviour::SYNC_REPLY;
		return msg_response;
	}

	ZST_EXPORT virtual ZstMessageReceipt send_async_message(ZstMessage* msg, const ZstTransportArgs& args){
		auto future = register_response(msg->id());

		//Hold on to the message id so we can clean up the promise in case we time out
		ZstMsgID id = msg->id();

		//Copy receive action so the lambda can reference it when the response arrives
		auto completed_action = args.on_recv_response;

		future.then([this, id, completed_action](ZstMessageFuture f) {
			ZstMsgKind status(ZstMsgKind::EMPTY);
			ZstMessageReceipt msg_response{ status };
			try {
				ZstMsgKind status = f.get().status;
				msg_response.status = status;
				msg_response.request_behaviour = ZstTransportRequestBehaviour::ASYNC_REPLY;
				completed_action(msg_response);
				return status;
			}
			catch (const ZstTimeoutException & e) {
				ZstLog::net(LogLevel::error, "Server async response timed out - {}", e.what());
				msg_response.status = ZstMsgKind::ERR_STAGE_TIMEOUT;
				completed_action(msg_response);
				enqueue_resolved_promise(id);
			}
			return status;
			});

		send_message_impl(msg, args);
		return ZstMessageReceipt{ ZstMsgKind::OK, ZstTransportRequestBehaviour::ASYNC_REPLY };
	}

	ZST_EXPORT std::shared_ptr<ZstEventDispatcher<std::shared_ptr<Adaptor_T> > >& msg_events() {
		return m_dispatch_events;
	}
	
	ZST_EXPORT bool is_active() {
		return m_is_active;
	}


private:
	bool m_is_active;

    std::shared_ptr<ZstEventDispatcher<std::shared_ptr<Adaptor_T> > > m_dispatch_events;
};

}
