#include "ZstMsgID.h"

ZstMsgID ZstMsgIDManager::s_last_id = 0;

void ZstMsgIDManager::init(const char * name, size_t len)
{
	ZstMsgIDManager::s_last_id = 0;
	const size_t prime = 31;
	for (size_t i = 0; i < len; ++i) {
		ZstMsgIDManager::s_last_id = name[i] + (ZstMsgIDManager::s_last_id * prime);
	}
}

ZstMsgID ZstMsgIDManager::next_id()
{
	ZstMsgIDManager::s_last_id++;
	return ZstMsgIDManager::s_last_id;
}
