#pragma once

#include <string>
#include <vector>
#include <memory>
#include "zmq.h"

#include "ZstUtils.h"
#include "ZstInstrument.h"

using namespace std;

namespace Showtime {

	class ZstSection
	{
	public:
		ZstSection(string name);
		DLL_EXPORT ~ZstSection();

		//Factory
		DLL_EXPORT static ZstSection* create_section(string name);

		// Creates a new instrument
		DLL_EXPORT ZstInstrument* create_instrument(string name);

		// Removes and destroys an instrument
		DLL_EXPORT void destroy_instrument(ZstInstrument& instrument);

		//List of all instruments owned by this section
		DLL_EXPORT vector<ZstInstrument*>& get_instruments();

	private:
		string m_name;
		vector<ZstInstrument*> m_instruments;
	};
}
