#include "ZstPerformerStageProxy.h"
#include <ZstLogging.h>

ZstPerformerStageProxy::ZstPerformerStageProxy(const std::string & name, const std::string & reliable_address, const std::string & unreliable_address) :
	ZstPerformer(name.c_str()),
	m_reliable_address(reliable_address),
	m_unreliable_address(unreliable_address)
{
}

ZstPerformerStageProxy::ZstPerformerStageProxy(const ZstPerformer & other, const std::string & reliable_address, const std::string & unreliable_address) : ZstPerformer(other)
{
	m_reliable_address = reliable_address;
	m_unreliable_address = unreliable_address;
}

const std::string & ZstPerformerStageProxy::reliable_address()
{
	return m_reliable_address;
}

const std::string & ZstPerformerStageProxy::unreliable_address()
{
	return m_unreliable_address;
}

void ZstPerformerStageProxy::add_subscriber_peer(ZstPerformerStageProxy * client)
{
	m_connected_subscriber_peers.insert(client->URI());
}

void ZstPerformerStageProxy::remove_subscriber_peer(ZstPerformerStageProxy * client)
{
	try{
		m_connected_subscriber_peers.erase(client->URI());
	} catch(std::out_of_range e){
		ZstLog::net(LogLevel::warn, "Peer {} not connected to {}", this->URI().path(), client->URI().path());
	}
}

bool ZstPerformerStageProxy::is_connected_to_subscriber_peer(ZstPerformerStageProxy * client)
{
	if(m_connected_subscriber_peers.find(client->URI()) != m_connected_subscriber_peers.end()){
		return true;
	}
	return false;
}

