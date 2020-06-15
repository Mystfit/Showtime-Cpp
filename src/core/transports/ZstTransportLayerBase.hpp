#pragma once

#include <string>

#include "ZstExports.h"
#include "ZstTransportHelpers.h"
#include "ZstExceptions.h"
#include "ZstPointerUtils.h"

#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "../ZstEventDispatcher.hpp"
#include "../ZstMessageSupervisor.hpp"
#include "../ZstMessagePool.hpp"
#include "ZstLogging.h"

namespace showtime {

class ZST_CLASS_EXPORTED ZstTransportLayerBase : public ZstMessageSupervisor
{
public:
    ZstTransportLayerBase() : 
        m_is_active(false),
        m_is_connected(false),
        m_async_pool(std::make_unique<boost::asio::thread_pool>(4)),
        m_event_condition(std::make_shared<ZstSemaphore>())
    {
    }

    virtual void init() {
        m_is_active = true;
        m_event_thread = boost::thread(boost::bind(&ZstTransportLayerBase::event_loop, this));
    }

    virtual void destroy() {
        m_event_thread.interrupt();
        m_event_condition->notify();
        m_event_thread.join();
        m_async_pool->join();
        m_is_active = false;
    }

    bool is_active() {
        return m_is_active;
    }

    bool is_connected() {
        return m_is_connected;
    }

    void set_connected(bool state) {
        m_is_connected = state;
    }

    // Transport events
    virtual void process_events() = 0;

protected:
    ZstMessageReceipt begin_send_message(std::shared_ptr<flatbuffers::FlatBufferBuilder>& buffer)
    {
        if (!is_connected())
            return ZstMessageReceipt{ Signal_ERR_STAGE_TIMEOUT };

        send_message_impl(buffer->GetBufferPointer(), buffer->GetSize(), ZstTransportArgs());
        return ZstMessageReceipt{ Signal_OK, ZstTransportRequestBehaviour::PUBLISH };
    }

    void begin_send_message(std::shared_ptr<flatbuffers::FlatBufferBuilder>& buffer, const ZstTransportArgs& args)
    {
        if (!is_connected())
            return;

        switch (args.msg_send_behaviour) {
            case ZstTransportRequestBehaviour::ASYNC_REPLY: {
                boost::asio::post(*m_async_pool, [this, buffer, args]() mutable {
                    this->blocking_send(buffer->GetBufferPointer(), buffer->GetSize(), args);
                });
                break;
            }
            case ZstTransportRequestBehaviour::SYNC_REPLY:
            {
                blocking_send(buffer->GetBufferPointer(), buffer->GetSize(), args);
                break;
            }
            case ZstTransportRequestBehaviour::PUBLISH: {
                send_message_impl(buffer->GetBufferPointer(), buffer->GetSize(), args);
                break;
            }
            default:
                Log::net(Log::Level::error, "Can't send message. Unknown message request behaviour");
                break;
        }
    }

    //Message sending implementation for the transport
    virtual void send_message_impl(const uint8_t* msg_buffer, size_t msg_buffer_size, const ZstTransportArgs& args) const = 0;

protected:
    std::shared_ptr<ZstSemaphore> m_event_condition;

private:
    void blocking_send(const uint8_t* msg_buffer, size_t msg_buffer_size, ZstTransportArgs args) {
        // Register message response
        auto msg_future = register_response(args.msg_ID);

        // Send message
        this->send_message_impl(msg_buffer, msg_buffer_size, args);

        // Wait for message promise to be fulfilled
        auto status = msg_future.wait_for(std::chrono::milliseconds(STAGE_TIMEOUT));
        auto msg = (status == std::future_status::ready) ? msg_future.get() : NULL;

        // Callbacks
        assert(args.msg_send_behaviour != ZstTransportRequestBehaviour::PUBLISH);
        args.on_recv_response(ZstMessageResponse{ msg, args.msg_send_behaviour });

        if (msg) {
            if (msg->has_promise()) {
                // We've been assigned responsibility to reset the response message
                this->release_owned_message(msg);
            }
        }
        else {
            Log::net(Log::Level::warn, "No reply received");
        }

        // Cleanup promises
        this->enqueue_resolved_promise(args.msg_ID);
    }

    void event_loop()
    {
        while (this->is_active()) {
            try {
                boost::this_thread::interruption_point();
               // m_event_condition->wait();

                // Process events from transports
                this->process_events();
            }
            catch (boost::thread_interrupted) {
                break;
            }
        }
    }

    bool m_is_active;
    bool m_is_connected;
    boost::thread m_event_thread;

    std::unique_ptr<boost::asio::thread_pool> m_async_pool;
};

template<typename Message_T, typename Adaptor_T>
class ZST_CLASS_EXPORTED ZstTransportLayer :
    public ZstTransportLayerBase,
    public ZstMessagePool<Message_T>,
    virtual public inheritable_enable_shared_from_this< ZstTransportLayer<Message_T, Adaptor_T> >
{
public:
    ZstTransportLayer() :
        m_dispatch_events(std::make_shared<ZstBlockingEventDispatcher<std::shared_ptr<Adaptor_T> > >())
    {
        //m_dispatch_events->set_wake_condition(m_event_condition);
    }
    virtual ~ZstTransportLayer() {}

    /**
     * @brief Let all listeners attached to this transport know that a message is available and
     *        manage the lifetime of a message.
     */
    virtual void dispatch_receive_event(std::shared_ptr<Message_T>& msg, ZstEventCallback on_complete) {
        m_dispatch_events->defer([msg](std::shared_ptr<Adaptor_T> adaptor) {
            std::static_pointer_cast<Adaptor_T>(adaptor)->on_receive_msg(std::static_pointer_cast<Message_T>(msg));
        }, [this, msg, on_complete](ZstEventStatus status) {
            auto base_msg = std::static_pointer_cast<ZstMessage>(msg);

            // If the has_promise flag is set early then someone else wants to take ownership of the message
            if (msg->has_promise()) {
                // Skip promise resolution - used for autojoin beacon promises
                return;
            }

            try {
                // Process the message
                process_message_response(base_msg, on_complete);
            }
            catch (std::out_of_range) {
                // No promise for message - can release message
                on_complete(status);
                this->release(msg);
            }
        });
    }

    virtual void process_events() override{
        this->cleanup_response_messages();
        m_dispatch_events->process_events();
    }

    std::shared_ptr<ZstBlockingEventDispatcher<std::shared_ptr<Adaptor_T> > >& msg_events() {
        return m_dispatch_events;
    }

private:
    std::shared_ptr<ZstBlockingEventDispatcher<std::shared_ptr<Adaptor_T> > > m_dispatch_events;
};


}
