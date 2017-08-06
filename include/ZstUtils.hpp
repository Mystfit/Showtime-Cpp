#pragma once
#include <vector>
#include <iostream>
#include "ZstExports.h"

namespace Utils{
    void str_split(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters = " ");
}

typedef char Str255[255];
