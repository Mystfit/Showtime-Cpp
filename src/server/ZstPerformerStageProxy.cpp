#include "ZstPerformerStageProxy.h"

ZstPerformerStageProxy::ZstPerformerStageProxy(const std::string & name, const std::string & ip_address) : 
	ZstPerformer(name.c_str()),
	m_address(ip_address)
{
}

const std::string & ZstPerformerStageProxy::ip_address()
{
	return m_address;
}
