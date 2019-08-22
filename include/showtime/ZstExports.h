#pragma once

#if defined _WIN32 || defined __CYGWIN__
    #ifdef __GNUC__
        #define ZST_API_EXPORTED __attribute__ ((dllexport))
        #define ZST_API_IMPORTED __attribute__ ((dllimport))
    #else
        #define ZST_API_EXPORTED __declspec(dllexport)
        #define ZST_API_IMPORTED __declspec(dllimport)
    #endif
#else
    #if __GNUC__ >= 4
        #define ZST_API_EXPORTED __attribute__ ((visibility ("default")))
        #define ZST_API_IMPORTED
    #else
        #define ZST_API_EXPORTED
        #define ZST_API_IMPORTED
    #endif
#endif

#ifdef ZST_EXPORT_CORE_API
#define ZST_EXPORT ZST_API_EXPORTED
#elif ZST_IMPORT_CORE_API
#define ZST_EXPORT ZST_API_IMPORTED
#else
#define ZST_EXPORT
#endif

#ifdef ZST_EXPORT_CLIENT_API
#define ZST_CLIENT_EXPORT ZST_API_EXPORTED
#elif ZST_IMPORT_CLIENT_API
#define ZST_CLIENT_EXPORT ZST_API_IMPORTED
#else
#define ZST_CLIENT_EXPORT
#endif

#ifdef ZST_EXPORT_SERVER_API
#define ZST_SERVER_EXPORT ZST_API_EXPORTED
#elif ZST_IMPORT_SERVER_API
#define ZST_SERVER_EXPORT ZST_API_IMPORTED
#else
#define ZST_SERVER_EXPORT
#endif

#ifdef ZST_EXPORT_ENTITY_API
#define ZST_ENTITY_EXPORT ZST_API_EXPORTED
#elif ZST_IMPORT_ENTITY_API
#define ZST_ENTITY_EXPORT ZST_API_IMPORTED
#else
#define ZST_ENTITY_EXPORT
#endif