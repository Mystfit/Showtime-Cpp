#include "ZstClientModule.h"
#include "ZstClient.h"

ZstClientModule::ZstClientModule()
{
}

ZstClientModule::ZstClientModule(ZstClient * client) :
	m_client(client)
{
}
//
//ZstClient * ZstClientModule::client()
//{
//	return m_client;
//}
