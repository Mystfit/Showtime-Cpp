#pragma once

#include <string>
#include <vector>
#include <memory>
#include "ZstInstrument.h"


#ifdef EXPORTS_API
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT __declspec(dllimport)
#endif

using namespace std;

#ifdef __cplusplus		//if C++ is used convert it to C to prevent C++'s name mangling of method names
extern "C"
{
#endif

	namespace Showtime {

		class ZstSection
		{
		public:
			ZstSection(string name);
			DLL_EXPORT ~ZstSection();

			//Factory
			DLL_EXPORT static unique_ptr<ZstSection> create_section(string name);

			// Creates a new instrument
			DLL_EXPORT shared_ptr<ZstInstrument> create_instrument(string name);

			// Removes and destroys an instrument
			DLL_EXPORT void destroy_instrument(ZstInstrument& instrument);

			//List of all instruments owned by this section
			DLL_EXPORT vector<shared_ptr<ZstInstrument>>& get_instruments();

		private:
			string m_name;
			vector<shared_ptr<ZstInstrument>> m_instruments;
		};
	}

#ifdef __cplusplus
}
#endif