#pragma once

#include <vector>
#include "ZstExports.h"
#include "ZstURI.h"

class ZstPlug;
class ZstInstrument {

public:
	ZST_EXPORT ~ZstInstrument();
	ZST_EXPORT ZstInstrument * create(ZstURI uri);

	virtual void compute(ZstPlug * plug);
	void add_plug(ZstPlug* plug);
	void remove_plug(ZstPlug* plug);

private:
	ZstInstrument(ZstURI uri);
	std::map<std::string, std::vector<ZstPlug*>> m_plugs;

};