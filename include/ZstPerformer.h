#pragma once
#include <map>
#include <string>
#include <vector>
#include "ZstURI.h"
#include "ZstExports.h"

class ZstPlug;
class ZstPerformer {
	public:
		ZstPerformer(std::string name);
		~ZstPerformer();
		ZST_EXPORT const char * get_name();
        ZST_EXPORT ZstPlug * get_plug_by_URI(ZstURI uri);

		void add_plug(ZstPlug* plug);
		void remove_plug(ZstPlug* plug);
	private:
		std::map<std::string, std::vector<ZstPlug*> > m_plugs;
		std::string m_name;
};
