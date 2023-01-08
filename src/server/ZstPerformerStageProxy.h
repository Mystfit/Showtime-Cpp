#pragma once

#include <string>
#include <unordered_set>
#include <boost/uuid/uuid.hpp>
#include <showtime/schemas/messaging/stage_message_generated.h>
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

	void add_listening_performer(ZstPerformerStageProxy* client, showtime::ConnectionType connection_type);
	void remove_listening_performer(ZstPerformerStageProxy* client, showtime::ConnectionType connection_type);
	bool is_sending_to(ZstPerformerStageProxy* client);
	bool is_sending_to(ZstPerformerStageProxy* client, showtime::ConnectionType connection_type);

	const boost::uuids::uuid & origin_endpoint_UUID();
	const std::weak_ptr<ZstStageTransport>& origin_transport();

private:
	std::string m_reliable_address;
	std::string m_reliable_public_address;
	std::string m_unreliable_address;
	std::string m_unreliable_public_address;

	std::unordered_set<ZstURI, ZstURIHash> m_connected_reliable_peers;
	std::unordered_set<ZstURI, ZstURIHash> m_connected_unreliable_peers;

	boost::uuids::uuid m_origin_endpoint_UUID;
	std::weak_ptr<ZstStageTransport> m_origin_transport;
};

}