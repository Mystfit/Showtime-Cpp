#pragma once  

#include <string>
#include <memory>
#include "ZstUtils.h"
#include "ZstPlug.h"

namespace Showtime {

	using namespace std;

	class ZstInstrument {
	public:
		ZstInstrument(string name);
		~ZstInstrument();

		//Plug factory
		DLL_EXPORT ZstPlug* create_plug(string name, ZstPlug::PlugMode plugMode);

		//Accessors
		DLL_EXPORT string get_name();

		DLL_EXPORT vector<ZstPlug*> get_outputs();
		DLL_EXPORT vector<ZstPlug*> get_inputs();


	private:
		string m_name;
		vector<ZstPlug*> m_outputs;
		vector<ZstPlug*> m_inputs;
	};
}

