#pragma once

#include <sstream>

class ZstStreamable {
public:
	virtual void write(std::stringstream & buffer) = 0;
	virtual void read(const char * buffer, size_t length, size_t & offset) = 0;
};