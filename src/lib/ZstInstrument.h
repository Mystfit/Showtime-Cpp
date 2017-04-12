#pragma once  

#include <string>
#include <memory>
#include "ZstExports.h"

#include "ZstPlug.h"

#ifdef EXPORTS_API
	#define ZST_EXPORT __declspec(dllexport)
#else
	#define ZST_EXPORT __declspec(dllimport)
#endif
  
#ifdef __cplusplus		//if C++ is used convert it to C to prevent C++'s name mangling of method names
extern "C"
{
#endif

	namespace Showtime {

		using namespace std;

		class ZstInstrument {
		public:
			ZstInstrument(string name);
			~ZstInstrument();

			//Plug factory
			ZST_EXPORT shared_ptr<ZstPlug> create_plug(string name, ZstPlug::PlugMode plugMode);

			//Accessors
			ZST_EXPORT string get_name();

			ZST_EXPORT vector<shared_ptr<ZstPlug>> get_outputs();
			ZST_EXPORT vector<shared_ptr<ZstPlug>> get_inputs();


		private:
			string m_name;
			vector<shared_ptr<ZstPlug>> m_outputs;
			vector<shared_ptr<ZstPlug>> m_inputs;
		};
	}

#ifdef __cplusplus
}
#endif