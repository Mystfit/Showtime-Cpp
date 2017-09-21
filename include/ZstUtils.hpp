#pragma once
#include <vector>
#include <iostream>
#include "ZstExports.h"

namespace Utils{
    void str_split(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters = " ");
	size_t hash_c_string(const char* p, size_t s);
}

//typedef char Str255[255];
