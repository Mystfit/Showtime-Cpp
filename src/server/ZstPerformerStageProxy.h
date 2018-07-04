#pragma once

#include <string>
#include <unordered_set>
#include <entities/ZstPerformer.h>

class ZstPerformerStageProxy : public ZstPerformer {
public:
	ZstPerformerStageProxy(const std::string & name, const std::string & ip_address);
	ZstPerformerStageProxy(const ZstPerformer & other, std::string address);

	const std::string & ip_address();

	void add_subscriber_peer(ZstPerformerStageProxy * client);
	void remove_subscriber_peer(ZstPerformerStageProxy * client);
	bool is_connected_to_subscriber_peer(ZstPerformerStageProxy * client);

private:
	std::string m_address;

	std::unordered_set<ZstURI, ZstURIHash> m_connected_subscriber_peers;
};
