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
		ZST_EXPORT std::string get_name();
        ZST_EXPORT ZstPlug * get_plug_by_URI(const ZstURI uri);

		void add_plug(ZstPlug* plug);
		void remove_plug(ZstPlug* plug);
	private:
		std::map<std::string, std::vector<ZstPlug*>> m_plugs;
		std::string m_name;
};


class ZstPerformerEventCallback {
public:
	ZST_EXPORT virtual ~ZstPerformerEventCallback() { std::cout << "Destroying performer event callback" << std::endl; }
	ZST_EXPORT virtual void run(ZstURI performer) { std::cout << "Running performer event callback" << std::endl; }
};
