#pragma once
#include <map>
#include <string>
#include <vector>
#include "ZstExports.h"

class ZstPlug;
class ZstPerformer {
	public:
		ZstPerformer(std::string name);
		~ZstPerformer();
		ZST_EXPORT std::string get_name();
		ZST_EXPORT std::vector<ZstPlug*> get_plugs();
		ZST_EXPORT std::vector<ZstPlug*> get_instrument_plugs(std::string instrument);
        ZST_EXPORT ZstPlug * get_plug_by_URI(std::string uri_str);

		void add_plug(ZstPlug* plug);
		void remove_plug(ZstPlug* plug);
	private:
		std::map<std::string, std::vector<ZstPlug*>> m_plugs;
		std::string m_name;
};
