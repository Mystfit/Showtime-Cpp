#pragma once

#include <sstream>
#include <ZstExports.h>
#include <ZstEvents.h>

class ZstSerialisable {
public:
	ZST_EXPORT virtual void write(std::stringstream & buffer) = 0;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) = 0;
};