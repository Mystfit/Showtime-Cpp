#pragma once

#include <stdint.h>
#include <stdlib.h>
#include "ZstExports.h"

typedef uint64_t ZstMsgID;

class ZstMsgIDManager {
public:
	ZST_EXPORT static void init(const char * name, size_t len);
	ZST_EXPORT static ZstMsgID next_id();
private:
	static ZstMsgID s_last_id;
};
