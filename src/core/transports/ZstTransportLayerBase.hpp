#pragma once

#include <string>

#include "ZstExports.h"
#include "ZstTransportHelpers.h"
#include "ZstExceptions.h"

#include "../ZstEventDispatcher.hpp"
#include "../ZstMessageSupervisor.hpp"
#include "../ZstMessagePool.hpp"

namespace showtime {

template<typename Message_T, typename Adaptor_T>
class ZST_CLASS_EXPORTED ZstTransportLayerBase : 
	public ZstMessageSupervisor,
    public ZstMessagePool<Message_T>
{
public:
	ZstTransportLayerBase() :
		ZstMessageSupervisor(std::make_shared<cf::time_watcher>(), STAGE_TIMEOUT),
		m_is_active(false),
		m_dispatch_events(std::make_shared<ZstEventDispatcher<std::shared_ptr<Adaptor_T> > >("transport events")){
	}
	virtual ~ZstTransportLayerBase() {}

	virtual void init() {
		m_is_active = true;
	}

	virtual void destroy() {}

	virtual void process_events(){
		m_dispatch_events->process_events();
        this->cleanup_response_messages();
	}

	std::shared_ptr<ZstEventDispatcher<std::shared_ptr<Adaptor_T> > >& msg_events() {
		return m_dispatch_events;
	}
	
	bool is_active() {
		return m_is_active;
	}
    
protected:
    ZstMessageReceipt begin_send_message(const uint8_t * msg_buffer, size_t msg_buffer_size)
    {
        send_message_impl(msg_buffer, msg_buffer_size, ZstTransportArgs());
        return ZstMessageReceipt{ Signal_OK, ZstTransportRequestBehaviour::PUBLISH };
    }
    
    ZstMessageReceipt begin_send_message(const uint8_t * msg_buffer, size_t msg_buffer_size, const ZstTransportArgs& args)
    {
        switch (args.msg_send_behaviour) {
            case ZstTransportRequestBehaviour::ASYNC_REPLY:
                return send_async_message(msg_buffer, msg_buffer_size, args);
                break;
            case ZstTransportRequestBehaviour::SYNC_REPLY:
            {
                auto receipt = send_sync_message(msg_buffer, msg_buffer_size, args);
                args.on_recv_response(receipt);
                return receipt;
                break;
            }
            case ZstTransportRequestBehaviour::PUBLISH: {
                send_message_impl(msg_buffer, msg_buffer_size, args);
                return ZstMessageReceipt{ Signal_OK, ZstTransportRequestBehaviour::PUBLISH };
                break;
            }
            default:
                ZstLog::net(LogLevel::error, "Can't send message. Unknown message request behaviour");
                break;
        }
        return ZstMessageReceipt{ Signal_EMPTY };
    }
    
    //Message sending implementation for the transport
    virtual void send_message_impl(const uint8_t * msg_buffer, size_t msg_buffer_size, const ZstTransportArgs & args) const = 0;
    
    virtual void dispatch_receive_event(Message_T* msg){
        m_dispatch_events->defer([msg](std::shared_ptr<Adaptor_T> adaptor) {
            adaptor->on_receive_msg(msg);
        }, [this, msg](ZstEventStatus status) {
            process_response(msg->id(), ZstMessageReceipt{ Signal::Signal_OK });
            this->release(msg);
        });
    }
    
    virtual void dispatch_receive_event(Message_T* msg, ZstEventCallback on_complete) {
        m_dispatch_events->defer([msg](std::shared_ptr<Adaptor_T> adaptor) {
            adaptor->on_receive_msg(msg);
        }, [this, msg, on_complete](ZstEventStatus status) {
            process_response(msg->id(), ZstMessageReceipt{ Signal::Signal_OK });
            on_complete(status);
            this->release(msg);
        });
    }

private:
    //Message supervision
    ZstMessageReceipt send_sync_message(const uint8_t * msg_buffer, size_t msg_buffer_size, const ZstTransportArgs& args){
        auto future = register_response(args.msg_ID);
        ZstMessageReceipt msg_response{ Signal_EMPTY };
        
        send_message_impl(msg_buffer, msg_buffer_size, args);
        try {
            //Blocking call on get
            msg_response = future.get();
        }
        catch (const ZstTimeoutException & e) {
            ZstLog::net(LogLevel::error, "Server response timed out", e.what());
            msg_response.status = Signal_ERR_STAGE_TIMEOUT;
        }
        
        enqueue_resolved_promise(args.msg_ID);
        msg_response.request_behaviour = ZstTransportRequestBehaviour::SYNC_REPLY;
        return msg_response;
    }
    
    ZstMessageReceipt send_async_message(const uint8_t * msg_buffer, size_t msg_buffer_size, const ZstTransportArgs& args){
        auto future = register_response(args.msg_ID);

        //Copy receive action so the lambda can reference it when the response arrives
        auto completed_action = args.on_recv_response;
        
        future.then([this, args, completed_action](ZstMessageFuture f) {
            Signal status(Signal_EMPTY);
            ZstMessageReceipt msg_response{ status };
            try {
                Signal status = f.get().status;
                msg_response.status = status;
                msg_response.request_behaviour = ZstTransportRequestBehaviour::ASYNC_REPLY;
                completed_action(msg_response);
                return status;
            }
            catch (const ZstTimeoutException & e) {
                ZstLog::net(LogLevel::error, "Server async response timed out - {}", e.what());
                msg_response.status = Signal_ERR_STAGE_TIMEOUT;
                completed_action(msg_response);
                enqueue_resolved_promise(args.msg_ID);
            }
            return status;
        });
        
        send_message_impl(msg_buffer, msg_buffer_size, args);
        return ZstMessageReceipt{ Signal_OK, ZstTransportRequestBehaviour::ASYNC_REPLY };
    }
    
	bool m_is_active;
    std::shared_ptr<ZstEventDispatcher<std::shared_ptr<Adaptor_T> > > m_dispatch_events;
};

}
