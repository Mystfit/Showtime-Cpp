#ifndef ZST_FORMAT
#define ZST_FORMAT
#pragma once

#ifdef USE_FMT_FORMAT
#include <fmt/format.h>
#define ZSTvformat fmt::vformat
#define ZSTformat fmt::format
#define ZSTmake_format_args fmt::make_format_args
#else
#include <format>
#define ZSTvformat std::vformat
#define ZSTformat std::format
#define ZSTmake_format_args std::make_format_args

#endif

#endif // ZST_FORMAT