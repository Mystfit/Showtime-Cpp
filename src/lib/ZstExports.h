#pragma once

#ifdef EXPORTS_API
#define ZST_EXPORT __declspec(dllexport)
#else
#define ZST_EXPORT __declspec(dllimport)
#endif
