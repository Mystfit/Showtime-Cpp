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

			//Constructor
			ZstPlug();

			//Plug factory
			DLL_EXPORT ZstPlug* create_plug(string name, string ownerName, PlugMode mode, const string args[], function<void(string)> callback);
			DLL_EXPORT string get_name();


		private:
			string m_name;
			string origin;
			PlugMode m_plug_mode;
			
			//Inputs
			string args[];
			
			//Outputs
		};


	}

#ifdef __cplusplus
}
#endif