#include <sstream>

#include "../core/ZstEventDispatcher.hpp"
#include "../core/ZstZMQRefCounter.h"
#include "ZstZMQClientTransport.h"
#include "nlohmann/json.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

namespace showtime {


ZstZMQClientTransport::ZstZMQClientTransport() : 
	m_server_sock(NULL),
	m_endpoint_UUID(random_generator()())
{
}

ZstZMQClientTransport::~ZstZMQClientTransport()
{
    //m_client_actor.stop_loop();
	destroy();
}

void ZstZMQClientTransport::init()
{
	ZstTransportLayerBase::init();

	//Init actor before attaching sockets
	m_client_actor.init("client_actor");

	//Dealer socket for server comms
	m_server_sock = zsock_new(ZMQ_DEALER);
	if (m_server_sock) {
		zsock_set_linger(m_server_sock, 0);
		m_client_actor.attach_pipe_listener(m_server_sock, s_handle_stage_router, this);
	}
	else {
		ZstLog::net(LogLevel::error, "Could not create client socket. Reason: {}", zmq_strerror(zmq_errno()));
		return;
	}

	//Set socket ID to identify socket with receivers
	zsock_set_identity(m_server_sock, boost::lexical_cast<std::string>(m_endpoint_UUID).c_str());

	//IO loop
	m_client_actor.start_loop();

	//Since we create a ZMQ socket, we need to ref count zmq
	zst_zmq_inc_ref_count();
}

void ZstZMQClientTransport::destroy()
{
	m_client_actor.stop_loop();
	if (m_server_sock) {
		//m_client_actor.remove_pipe_listener(m_server_sock);
		zsock_destroy(&m_server_sock);
		m_server_sock = NULL;
		zst_zmq_dec_ref_count();
	}
	ZstTransportLayerBase::destroy();
}

void ZstZMQClientTransport::connect(const std::string & address)
{
	std::stringstream addr;
    addr << "tcp://" << address; // << ":" << STAGE_ROUTER_PORT;
	m_server_addr = addr.str();

	if(m_server_sock)
		zsock_connect(m_server_sock, "%s", m_server_addr.c_str());
}

void ZstZMQClientTransport::disconnect()
{
	if (m_server_sock)
		zsock_disconnect(m_server_sock, "%s", m_server_addr.c_str());
}
    
ZstMessageReceipt ZstZMQClientTransport::send_msg(Content message_type, flatbuffers::Offset<void> message_content, flatbuffers::FlatBufferBuilder & buffer_builder, const ZstTransportArgs& args)
{
    // Make a copy of the transport args so we can generate a new message ID if required
    auto copy_args = args;
    copy_args.msg_ID = (args.msg_ID > 0) ? args.msg_ID : ZstMsgIDManager::next_id();
    
    // Create the stage message
    auto stage_msg = CreateStageMessage(buffer_builder, message_type, message_content, copy_args.msg_ID);
    FinishStageMessageBuffer(buffer_builder, stage_msg);
    
    begin_send_message(buffer_builder.GetBufferPointer(), buffer_builder.GetSize(), args);
}


void ZstZMQClientTransport::send_message_impl(const uint8_t * msg_buffer, size_t msg_buffer_size, const ZstTransportArgs & args) const
{
	zmsg_t * m = zmsg_new();

	//Insert empty frame at front of message to seperate between router sender hops and payloads
	zframe_t * spacer = zframe_new_empty();
	zmsg_prepend(m, &spacer);

	//Encode message as json
    zmsg_addmem(m, msg_buffer, msg_buffer_size);

	//Sending and errors
	int result = m_client_actor.send_to_socket(m_server_sock, m);
	if (result != 0) {
		int err = zmq_errno();
		if (err > 0) {
			ZstLog::net(LogLevel::error, "Client message sending error: {}", zmq_strerror(err));
		}
	}
}

void ZstZMQClientTransport::sock_recv(zsock_t* socket, bool pop_first)
{
	if (!is_active())
		return;

	zmsg_t * recv_msg = zmsg_recv(socket);
	if (recv_msg) {
		if (pop_first) {
			auto empty = zmsg_pop(recv_msg);
			zframe_destroy(&empty);
		}
        auto msg_data = zmsg_pop(recv_msg);

        if(msg_data){
            ZstStageMessage * stage_msg = get_msg();
            stage_msg->init(GetStageMessage(zframe_data(msg_data)));
            
            // Send message to submodules
            dispatch_receive_event(stage_msg, [this, stage_msg, msg_data](ZstEventStatus s) mutable{
                // Frame cleanup
                zframe_destroy(&msg_data);
                this->release(stage_msg);
            });
        }
        
        // Message Cleanup
		zmsg_destroy(&recv_msg);
	}
}

int ZstZMQClientTransport::s_handle_stage_router(zloop_t * loop, zsock_t * sock, void * arg)
{
	ZstZMQClientTransport * transport = (ZstZMQClientTransport*)arg;
	transport->sock_recv(sock, true);
	return 0;
}

}
