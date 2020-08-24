#pragma once

#if defined _WIN32 || defined __CYGWIN__
    #ifdef __GNUC__
        #define ZST_API_EXPORTED __attribute__ ((dllexport))
        #define ZST_API_IMPORTED __attribute__ ((dllimport))
		#define ZST_CLASS_EXPORTED ZST_API_EXPORTED		
    #else
        #define ZST_API_EXPORTED __declspec(dllexport)
        #define ZST_API_IMPORTED __declspec(dllimport)
		#define ZST_CLASS_EXPORTED
    #endif
#else
    #if __GNUC__ >= 4
        #define ZST_API_EXPORTED __attribute__ ((visibility ("default")))
        #define ZST_API_IMPORTED
		#define ZST_CLASS_EXPORTED ZST_API_EXPORTED
    #else
        #define ZST_API_EXPORTED
        #define ZST_API_IMPORTED
		#define ZST_CLASS_EXPORTED
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

#ifdef ZST_EXPORT_PLUGIN_API
#define ZST_PLUGIN_EXPORT ZST_API_EXPORTED
#elif ZST_IMPORT_PLUGIN_API
#define ZST_PLUGIN_EXPORT ZST_API_IMPORTED
#else
#define ZST_PLUGIN_EXPORT
#endif