#pragma once

#include <string>
#include <unordered_set>
#include <entities/ZstPerformer.h>

class ZstPerformerStageProxy : public ZstPerformer {
public:
	ZstPerformerStageProxy(const std::string & name, const std::string & reliable_address, const std::string & unreliable_address);
	ZstPerformerStageProxy(const ZstPerformer & other, const std::string & reliable_address, const std::string & unreliable_address);

	const std::string & reliable_address();
	const std::string & unreliable_address();

	void add_subscriber_peer(ZstPerformerStageProxy * client);
	void remove_subscriber_peer(ZstPerformerStageProxy * client);
	bool is_connected_to_subscriber_peer(ZstPerformerStageProxy * client);

private:
	std::string m_reliable_address;
	std::string m_unreliable_address;

	std::unordered_set<ZstURI, ZstURIHash> m_connected_subscriber_peers;
};
