#include "ZstClientModule.h"

ZstClientModule::ZstClientModule(ZstClient * client) : 
	m_client(client)
{
}

ZstClient * ZstClientModule::client()
{
	return m_client;
}
