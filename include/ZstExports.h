#pragma once

#ifdef EXPORTS_CORE_API
#define ZST_EXPORT __declspec(dllexport)
#elif IMPORTS_CORE_API
#define ZST_EXPORT __declspec(dllimport)
#else
#define ZST_EXPORT
#endif

#ifdef EXPORTS_CLIENT_API
#define ZST_CLIENT_EXPORT __declspec(dllexport)
#elif IMPORTS_CLIENT_API
#define ZST_CLIENT_EXPORT __declspec(dllimport)
#else
#define ZST_CLIENT_EXPORT
#endif
