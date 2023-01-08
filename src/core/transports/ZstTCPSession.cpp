#include "ZstTCPSession.h"
#include <boost/thread/future.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/write.hpp>

using namespace boost::asio;

namespace showtime {

	ZstTCPSession::ZstTCPSession(std::shared_ptr<ZstTCPGraphTransport> transport, io_context& context) :
	m_ctx(context),
	m_socket(nullptr),
	m_origin_endpoint_UUID(boost::uuids::nil_uuid()),
	m_recv_buf(),
	m_transport(transport)
{
}

ZstTCPSession::~ZstTCPSession()
{
	m_socket->shutdown(boost::asio::socket_base::shutdown_both);
	m_socket->close();
}

void ZstTCPSession::listen()
{
	// Start reading messages
	do_read();

	// Queue up write message handler
	boost::asio::post(m_ctx, boost::bind(&ZstTCPSession::do_write, shared_from_this()));
}

void ZstTCPSession::do_read()
{
	m_socket->async_receive(boost::asio::buffer(m_recv_buf), boost::bind(
		&ZstTCPSession::on_read,
		this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::detail::placeholder<2>::get()
	));
}

void ZstTCPSession::queue_write(std::shared_ptr<flatbuffers::FlatBufferBuilder> buffer_builder)
{
	m_out_messages.enqueue(buffer_builder);
}

void ZstTCPSession::do_write()
{
	on_write(boost::system::error_code(), 0);
}

void ZstTCPSession::on_write(const boost::system::error_code& error, std::size_t bytes_transferred) {
	std::shared_ptr<flatbuffers::FlatBufferBuilder> message;
	m_out_messages.wait_dequeue(message);
	
	// Send next message
	m_socket->async_send(boost::asio::buffer(message->GetBufferPointer(), message->GetSize()), boost::bind(&ZstTCPSession::on_write, 
		shared_from_this(), 
		boost::asio::placeholders::error,
		boost::asio::placeholders::detail::placeholder<2>::get())
	);
}

void ZstTCPSession::on_read(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	int length = bytes_transferred;

	flatbuffers::Verifier verifier((uint8_t*)m_recv_buf.data(), length);
	if (length <= 0 || !VerifyGraphMessageBuffer(verifier)) {
		// Go back to listening
		m_socket->async_receive(boost::asio::buffer(m_recv_buf), boost::bind(
			&ZstTCPSession::on_read,
			this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)
		);
		return;
	}

	// Link contents of the receive buffer into a message
	auto perf_msg = m_transport->get_msg();
	auto owner = std::static_pointer_cast<ZstGraphTransport>(m_transport);
	perf_msg->init(GetGraphMessage(m_recv_buf.data()), owner);

	//Publish message to other modules
	m_transport->dispatch_receive_event(perf_msg, [perf_msg, this](ZstEventStatus s) mutable {
		// Cleanup
		m_transport->release(perf_msg);

		// Listen for more messages again
		do_read();
	});
}

}