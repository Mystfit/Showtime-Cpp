#pragma once

#include <sstream>
#include <ZstExports.h>
#include <nlohmann/json_fwd.hpp>

using json = nlohmann::json;

class ZstSerialisable {
public:
	ZST_EXPORT virtual void write(std::stringstream & buffer) const = 0;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) = 0;
	ZST_EXPORT virtual json as_json() const;
	ZST_EXPORT std::string as_json_str() const;
	ZST_EXPORT void from_json_str(std::string & str);
	ZST_EXPORT void from_json_str(const char * str);
	ZST_EXPORT virtual void write_json(json & buffer) const = 0;
	ZST_EXPORT virtual void read_json(const json & buffer) = 0;
};
