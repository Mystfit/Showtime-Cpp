#pragma once

#include <vector>
#include "ZstExports.h"
#include "ZstEntityBase.h"

class ZstPlugDataEventCallback;

#define COMPONENT_TYPE "filter"

class ZstComponent : public ZstEntityBase {
public:
	friend class ComputeCallback;
	ZST_EXPORT ZstComponent(const char * entity_type, const char * name);
	ZST_EXPORT ZstComponent(const char * entity_type, const char * name, ZstEntityBase * parent);
	ZST_EXPORT ~ZstComponent();
	ZST_EXPORT virtual void init() override;
	ZST_EXPORT virtual ZstPlug * get_plug_by_URI(const ZstURI uri);
	ZST_EXPORT virtual void compute(ZstInputPlug * plug);

	ZST_EXPORT virtual ZstInputPlug * create_input_plug(const char* name, ZstValueType val_type);
	ZST_EXPORT virtual ZstOutputPlug * create_output_plug(const char* name, ZstValueType val_type);
	ZST_EXPORT virtual void remove_plug(ZstPlug *plug);

private:
	std::vector<ZstPlug*> m_plugs;
	ComputeCallback * m_compute_callback;
};


class ComputeCallback : public ZstPlugDataEventCallback {
public:
	ZST_EXPORT void set_target_filter(ZstComponent * component);
	ZST_EXPORT void run(ZstInputPlug * plug) override;
private:
	ZstComponent * m_component;
};
