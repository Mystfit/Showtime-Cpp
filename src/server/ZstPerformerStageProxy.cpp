#include <boost/uuid/nil_generator.hpp>

#include "ZstPerformerStageProxy.h"
#include "ZstLogging.h"

using namespace boost::uuids;

namespace showtime {

ZstPerformerStageProxy::ZstPerformerStageProxy(const Entity* performer, const std::string& reliable_address, const std::string& unreliable_address, const uuid& endpoint_UUID) :
	ZstPerformer(performer),
	m_reliable_address(reliable_address),
	m_unreliable_address(unreliable_address),
	m_endpoint_UUID(endpoint_UUID)
{
	this->set_activated();
	this->set_proxy();
}

ZstPerformerStageProxy::ZstPerformerStageProxy(const ZstPerformerStageProxy& other) :
	ZstPerformer(other),
	m_reliable_address(other.m_reliable_address),
	m_unreliable_address(other.m_unreliable_address),
	m_endpoint_UUID(other.m_endpoint_UUID)
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
		ZstLog::server(LogLevel::warn, "Client {} not subscribed to {}", this->URI().path(), client->URI().path());
	}
}

bool ZstPerformerStageProxy::has_connected_subscriber(ZstPerformerStageProxy* client)
{
	if (m_connected_subscriber_peers.find(client->URI()) != m_connected_subscriber_peers.end()) {
		return true;
	}
	return false;
}

const boost::uuids::uuid & ZstPerformerStageProxy::endpoint_UUID()
{
	return m_endpoint_UUID;
}

}