#pragma once

#include <string>
#include <unordered_set>
#include <boost/uuid/uuid.hpp>

#include "entities/ZstPerformer.h"

namespace showtime {

class ZstPerformerStageProxy : public ZstPerformer {
public:
	ZstPerformerStageProxy(const Performer* performer, const std::string& reliable_address, const std::string& unreliable_address, const boost::uuids::uuid& endpoint_UUID);
	ZstPerformerStageProxy(const ZstPerformerStageProxy& other);

	const std::string& reliable_address();
	const std::string& unreliable_address();

	void add_subscriber(ZstPerformerStageProxy* client);
	void remove_subscriber(ZstPerformerStageProxy* client);
	bool has_connected_subscriber(ZstPerformerStageProxy* client);

	const boost::uuids::uuid & endpoint_UUID();

private:
	std::string m_reliable_address;
	std::string m_unreliable_address;

	std::unordered_set<ZstURI, ZstURIHash> m_connected_subscriber_peers;
	boost::uuids::uuid m_endpoint_UUID;
};

}