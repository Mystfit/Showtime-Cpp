#include <boost/uuid/nil_generator.hpp>

#include "../core/transports/ZstStageTransport.h"
#include "ZstPerformerStageProxy.h"
#include <showtime/ZstLogging.h>

using namespace boost::uuids;

namespace showtime {

ZstPerformerStageProxy::ZstPerformerStageProxy(
		const Performer* performer, 
		const std::string& reliable_address, 
		const std::string& unreliable_address, 
		const std::string& unreliable_public_address, 
		const uuid& origin_endpoint_UUID, 
		const std::weak_ptr<ZstStageTransport>& origin_transport) :
	ZstPerformer(performer),
	m_reliable_address(reliable_address),
	m_unreliable_address(unreliable_address),
	m_unreliable_public_address(unreliable_public_address),
	m_origin_endpoint_UUID(origin_endpoint_UUID),
	m_origin_transport(origin_transport)
{
	this->set_activated();
	this->set_proxy();
}

ZstPerformerStageProxy::ZstPerformerStageProxy(const ZstPerformerStageProxy& other) :
	ZstPerformer(other),
	m_reliable_address(other.m_reliable_address),
	m_unreliable_address(other.m_unreliable_address),
	m_origin_endpoint_UUID(other.m_origin_endpoint_UUID),
	m_origin_transport(other.m_origin_transport)

{
	this->set_activated();
	this->set_proxy();
}

const std::string& ZstPerformerStageProxy::reliable_address()
{
	return m_reliable_address;
}

const std::string& ZstPerformerStageProxy::unreliable_address()
{
	return m_unreliable_address;
}

const std::string& ZstPerformerStageProxy::unreliable_public_address()
{
	return m_unreliable_public_address;
}

void ZstPerformerStageProxy::add_subscriber(ZstPerformerStageProxy* client)
{
	m_connected_subscriber_peers.insert(client->URI());
}

void ZstPerformerStageProxy::remove_subscriber(ZstPerformerStageProxy* client)
{
	try {
		m_connected_subscriber_peers.erase(client->URI());
	}
	catch (std::out_of_range e) {
		Log::server(Log::Level::warn, "Client {} not subscribed to {}", this->URI().path(), client->URI().path());
	}
}

bool ZstPerformerStageProxy::has_connected_subscriber(ZstPerformerStageProxy* client)
{
	if (m_connected_subscriber_peers.find(client->URI()) != m_connected_subscriber_peers.end()) {
		return true;
	}
	return false;
}

const boost::uuids::uuid & ZstPerformerStageProxy::origin_endpoint_UUID()
{
	return m_origin_endpoint_UUID;
}

const std::weak_ptr<ZstStageTransport>& ZstPerformerStageProxy::origin_transport()
{
	return m_origin_transport;
}

}