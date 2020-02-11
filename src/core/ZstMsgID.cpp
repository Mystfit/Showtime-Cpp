#include <boost/uuid/uuid_generators.hpp>
#include "ZstMsgID.h"
#include "ZstConstants.h"

void ZstMsgIDManager::init(const char * name, size_t len)
{
    boost::uuids::random_generator()();
}

ZstMsgID ZstMsgIDManager::next_id()
{
	return boost::uuids::random_generator()();
}
