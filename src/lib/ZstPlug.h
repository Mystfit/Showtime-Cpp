#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include "ZstUtils.h"
 
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
		DLL_EXPORT string get_name();
		DLL_EXPORT PlugMode get_mode();

	private:
		string m_name;
		PlugMode m_plug_mode;
			
		//Inputs
		vector<string> m_args;
			
		//Outputs
		string m_output;
		bool m_outputReady = false;
	};


}
