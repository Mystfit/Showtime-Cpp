#pragma once

#ifdef EXPORTS_API
#define ZST_EXPORT __declspec(dllexport)
#elseif IMPORTS_API
#define ZST_EXPORT __declspec(dllimport)
#else
#define ZST_EXPORT
#endif
