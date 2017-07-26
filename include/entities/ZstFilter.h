#pragma once

#include "ZstEntityBase.h"

class ZstFilter : public virtual ZstEntityBase {
public:
	ZST_EXPORT ZstFilter();
	ZST_EXPORT ZstFilter(const char * entity_type, const char * name, ZstURI parent);
	ZST_EXPORT virtual ZstInputPlug * create_input_plug(const char* name, ZstValueType val_type);
	ZST_EXPORT virtual ZstOutputPlug * create_output_plug(const char* name, ZstValueType val_type);
	ZST_EXPORT virtual void remove_plug(ZstPlug *plug);
	ZST_EXPORT virtual ZstPlug * get_plug_by_URI(const ZstURI uri);

protected:
	ZST_EXPORT virtual void compute(ZstInputPlug * plug) {};

private:
	std::vector<ZstPlug*> m_plugs;
};