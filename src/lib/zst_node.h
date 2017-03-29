#pragma once  
 
#ifdef EXPORTS_API
	#define DLL_EXPORT __declspec(dllexport)
#else
	#define DLL_EXPORT __declspec(dllimport)
#endif
  
#ifdef __cplusplus		//if C++ is used convert it to C to prevent C++'s name mangling of method names
extern "C"
{
#endif

	namespace Showtime {

		class ZstNode {
		public:
			ZstNode();
			DLL_EXPORT ZstNode* createNode();
		};
	}

#ifdef __cplusplus
}
#endif