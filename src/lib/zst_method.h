#pragma once

#include <string>
#include <map>
#include <functional>

 
#ifdef EXPORTS_API
	#define DLL_EXPORT __declspec(dllexport)
#else
	#define DLL_EXPORT __declspec(dllimport)
#endif
  
#ifdef __cplusplus		//if C++ is used convert it to C to prevent C++'s name mangling of method names

using namespace std;

extern "C"
{
#endif

	namespace Showtime {

		class ZstPlug {

		public:
			enum PlugMode {
				READABLE = 0,
				WRITEABLE,
				QUERYABLE
			};

			ZstPlug(string name, string ownerName, PlugMode mode, const string args[], function<void(string)> callback);

		protected:
		private:
			PlugMode m_plugMode;
		};


	}

#ifdef __cplusplus
}
#endif