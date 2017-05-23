#pragma once
#include <map>
#include "ZstExports.h"
#include "ZstPlug.h"

class ZstPerformer {
	public:
		ZstPerformer(std::string name);
		~ZstPerformer();
		ZST_EXPORT std::string get_name();
		ZST_EXPORT std::vector<ZstPlug*> get_plugs();
		ZST_EXPORT std::vector<ZstPlug*> get_instrument_plugs(std::string instrument);
		void add_plug(ZstPlug* plug);
		void remove_plug(ZstPlug* plug);
	private:
		std::map<std::string, std::vector<ZstPlug*>> m_plugs;
		std::string m_name;
};