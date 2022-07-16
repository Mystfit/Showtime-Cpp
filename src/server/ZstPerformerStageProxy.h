#pragma once

#include <string>
#include <unordered_set>
#include <boost/uuid/uuid.hpp>

#include <showtime/entities/ZstPerformer.h>

// Forwards
class ZstStageTransport;

namespace showtime {

class ZstPerformerStageProxy : public ZstPerformer {
public:
	ZstPerformerStageProxy(const Performer* performer, const std::string& reliable_address, const std::string& reliable_public_address, const std::string& unreliable_address, const std::string& unreliable_public_address, const boost::uuids::uuid& origin_endpoint_UUID, const std::weak_ptr<ZstStageTransport>& origin_transport);
	ZstPerformerStageProxy(const ZstPerformerStageProxy& other);

	const std::string& reliable_address();
	const std::string& reliable_public_address();
	const std::string& unreliable_address();
	const std::string& unreliable_public_address();

	void add_subscriber(ZstPerformerStageProxy* client);
	void remove_subscriber(ZstPerformerStageProxy* client);
	bool has_connected_subscriber(ZstPerformerStageProxy* client);

	const boost::uuids::uuid & origin_endpoint_UUID();
	const std::weak_ptr<ZstStageTransport>& origin_transport();

private:
	std::string m_reliable_address;
	std::string m_reliable_public_address;
	std::string m_unreliable_address;
	std::string m_unreliable_public_address;

	std::unordered_set<ZstURI, ZstURIHash> m_connected_subscriber_peers;
	boost::uuids::uuid m_origin_endpoint_UUID;
	std::weak_ptr<ZstStageTransport> m_origin_transport;
};

}