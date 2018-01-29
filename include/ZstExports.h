#pragma once

#ifdef EXPORT_CORE_API
#define ZST_EXPORT __declspec(dllexport)
#elif IMPORT_CORE_API
#define ZST_EXPORT __declspec(dllimport)
#else
#define ZST_EXPORT
#endif

#ifdef EXPORT_CLIENT_API
#define ZST_CLIENT_EXPORT __declspec(dllexport)
#elif IMPORT_CLIENT_API
#define ZST_CLIENT_EXPORT __declspec(dllimport)
#else
#define ZST_CLIENT_EXPORT
#endif

#ifdef EXPORT_SERVER_API
#define ZST_SERVER_EXPORT __declspec(dllexport)
#elif IMPORT_SERVER_API
#define ZST_SERVER_EXPORT __declspec(dllimport)
#else
#define ZST_SERVER_EXPORT
#endif