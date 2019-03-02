#pragma once

#ifndef SWIG
#include "nlohmann/json.hpp"
#endif
using nlohmann::json;
//using json = nlohmann::json;
#include "ZstExports.h"

class ZstSerialisable {
public:
	ZST_EXPORT virtual json as_json() const;
	ZST_EXPORT std::string as_json_str() const;
	ZST_EXPORT void from_json_str(std::string & str);
	ZST_EXPORT void from_json_str(const char * str);
	ZST_EXPORT virtual void write_json(json & buffer) const = 0;
	ZST_EXPORT virtual void read_json(const json & buffer) = 0;
};