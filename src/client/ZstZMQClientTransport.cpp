#include <sstream>

#include "../core/ZstEventDispatcher.hpp"
#include "../core/ZstZMQRefCounter.h"
#include "../core/ZstStageMessage.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include <czmq.h>
#include "ZstZMQClientTransport.h"

namespace showtime {


ZstZMQClientTransport::ZstZMQClientTransport() : 
	m_server_sock(NULL),
	m_endpoint_UUID(random_generator()())
{
}

ZstZMQClientTransport::~ZstZMQClientTransport()
{
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
		m_client_actor.remove_pipe_listener(m_server_sock);
		zsock_destroy(&m_server_sock);
		zst_zmq_dec_ref_count();
		m_server_sock = NULL;
	}

	ZstTransportLayerBase::destroy();
}

void ZstZMQClientTransport::connect(const std::string & address)
{
	std::stringstream addr;
    addr << "tcp://" << address; // << ":" << STAGE_ROUTER_PORT;
	m_server_addr = addr.str();

	if (m_server_sock) {
		auto result = zsock_connect(m_server_sock, "%s", m_server_addr.c_str());
		if (result == 0) {
			set_connected(true);
		}
		else {
			ZstLog::net(LogLevel::error, "Client connection error: {}", zmq_strerror(result));
		}
	}
}

void ZstZMQClientTransport::disconnect()
{
	if (m_server_sock) {
		zsock_disconnect(m_server_sock, "%s", m_server_addr.c_str());
		set_connected(false);
	}
}

void ZstZMQClientTransport::send_message_impl(const uint8_t * msg_buffer, size_t msg_buffer_size, const ZstTransportArgs & args) const
{
	zmsg_t * m = zmsg_new();

	//Insert empty frame at front of message to seperate between router sender hops and payloads
	zframe_t * spacer = zframe_new_empty();
	zmsg_prepend(m, &spacer);

	//Encode message from flatbuffers to bytes
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

void ZstZMQClientTransport::sock_recv(zsock_t* socket)
{
	if (!is_active())
		return;

	zmsg_t * recv_msg = zmsg_recv(socket);
	if (recv_msg) {
		// Pop spacer frame off
		auto empty = zmsg_pop(recv_msg);
        auto msg_data = zmsg_pop(recv_msg);

        if(msg_data){
            auto stage_msg = get_msg();
            stage_msg->init(GetStageMessage(zframe_data(msg_data)));

			//ZstLog::net(LogLevel::debug, "Receiving msg with ID: {}", stage_msg->id());
            
            // Send message to submodules
            dispatch_receive_event(stage_msg, [stage_msg, msg_data](ZstEventStatus s) mutable{
                // Frame cleanup
                zframe_destroy(&msg_data);
            });
        }
        
        // Message Cleanup
		zframe_destroy(&empty);
		zmsg_destroy(&recv_msg);
	}
}

int ZstZMQClientTransport::s_handle_stage_router(zloop_t * loop, zsock_t * sock, void * arg)
{
	ZstZMQClientTransport * transport = (ZstZMQClientTransport*)arg;
	transport->sock_recv(sock);
	return 0;
}

}
