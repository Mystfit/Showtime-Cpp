#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <boost/uuid/uuid.hpp>

#include <showtime/ZstExports.h>

typedef boost::uuids::uuid ZstMsgID;

class ZstMsgIDManager {
public:
	ZST_EXPORT static void init(const char * name, size_t len);
	ZST_EXPORT static ZstMsgID next_id();
private:
	static ZstMsgID s_last_id;
};
