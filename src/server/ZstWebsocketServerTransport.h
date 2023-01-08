#pragma once

#include "../core/ZstIOLoop.h"
#include "../core/ZstStageMessage.h"
#include "../core/transports/ZstStageTransport.h"

#include <memory>
#include <unordered_map>
#include <boost/uuid/uuid.hpp>
#include <boost/thread.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <boost/container_hash/hash.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using namespace boost::uuids;

namespace showtime {

//Forwards
class ZstWebsocketSession;


//Typedefs
typedef std::shared_ptr<ZstWebsocketSession> ZstWebsocketSessionPtr;


class ZstWebsocketServerTransport : public ZstStageTransport
{
	friend class ZstWebsocketSession;
public:
	ZstWebsocketServerTransport(ZstIOLoop & io);
	~ZstWebsocketServerTransport();
	void init() override;
	void destroy() override;
	virtual int bind(const std::string& address) override;
	static void fail(beast::error_code ec, char const* what);

private:
	void send_message_impl(std::shared_ptr<flatbuffers::FlatBufferBuilder> buffer_builder, const ZstTransportArgs& args) const override;

	void do_accept();
	void on_accept(beast::error_code ec, tcp::socket socket);

	boost::thread m_io_thread;
	tcp::acceptor m_acceptor;
	boost::asio::io_context& m_ioc;
	std::unordered_map< uuid, std::shared_ptr<ZstWebsocketSession>, boost::hash<boost::uuids::uuid> > m_sessions;
};

}
