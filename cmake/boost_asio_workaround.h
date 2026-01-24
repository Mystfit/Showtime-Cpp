// Workaround header for MSVC compatibility issues with Boost and CZMQ
// Force-include this header via /FI compiler flag to ensure it's processed first
//
// Issues addressed:
// 1. Boost.Asio 1.90 bug: std::strtoull/strtoll get substituted to std::_strtoui64/std::_strtoi64
//    which don't exist in the std namespace (they're in global namespace)
// 2. CZMQ headers use off_t but don't define it for MSVC
// 3. WinSock2.h must be included before WinSock.h (Boost.Asio requirement)

#pragma once

#if defined(_MSC_VER)

// Include WinSock2 FIRST to prevent "WinSock.h has already been included" errors
// This must happen before any other Windows headers or Boost headers
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#endif

// Define off_t for CZMQ headers BEFORE sys/types.h gets included
// This prevents sys/types.h from defining it as 'long' (32-bit)
// We need 64-bit off_t for large file support
#if !defined(_OFF_T_DEFINED)
typedef long long _off_t;
typedef long long off_t;
#define _OFF_T_DEFINED
#endif

#if _MSC_VER >= 1940
// Force include <cstdlib> first to get the MSVC intrinsics defined
#include <cstdlib>
#include <stdlib.h>

// MSVC substitutes strtoull/strtoll with _strtoui64/_strtoi64, but only in global namespace
// Add them to std namespace where Boost.Asio expects them
namespace std {
    using ::_strtoui64;
    using ::_strtoi64;
}
#endif

#endif
