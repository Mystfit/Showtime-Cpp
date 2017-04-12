#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include "ZstExports.h"

 
#ifdef EXPORTS_API
	#define ZST_EXPORT __declspec(dllexport)
#else
	#define ZST_EXPORT __declspec(dllimport)
#endif
  
#ifdef __cplusplus		//if C++ is used convert it to C to prevent C++'s name mangling of method names


extern "C"
{
#endif
	using namespace std;
	namespace Showtime {

		class ZstPlug {

		public:
			enum PlugMode {
				READABLE = 0,
				WRITEABLE
			};

			//Constructor
			ZstPlug(string name, PlugMode mode);
			
			//Accessors
			ZST_EXPORT string get_name();
			ZST_EXPORT PlugMode get_mode();

		private:
			string m_name;
			PlugMode m_plug_mode;
			
			//Inputs
			vector<string> m_args;
			
			//Outputs
			string m_output;
			bool m_output_ready = false;
		};


	}

#ifdef __cplusplus
}
#endif