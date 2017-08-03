#pragma once

#include "ZstEntityBase.h"

#define FILTER_TYPE "filter"

class FilterComputeCallback;

class ZstFilter : public virtual ZstEntityBase {
public:
	friend class FilterComputeCallback;
	ZST_EXPORT ZstFilter(const char * name) : ZstEntityBase(FILTER_TYPE, name) { init(); };
	ZST_EXPORT ZstFilter(const char * name, ZstEntityBase * parent) : ZstEntityBase(FILTER_TYPE, name, parent) { init(); };
	ZST_EXPORT ~ZstFilter();
	ZST_EXPORT virtual void init() override;

	ZST_EXPORT virtual ZstInputPlug * create_input_plug(const char* name, ZstValueType val_type);
	ZST_EXPORT virtual ZstOutputPlug * create_output_plug(const char* name, ZstValueType val_type);
	ZST_EXPORT virtual void remove_plug(ZstPlug *plug);
	ZST_EXPORT virtual ZstPlug * get_plug_by_URI(const ZstURI uri);
protected:
	ZST_EXPORT virtual void compute(ZstInputPlug * plug) {};

private:
	std::vector<ZstPlug*> m_plugs;
	FilterComputeCallback * m_compute_callback;
};


class FilterComputeCallback : public ZstPlugDataEventCallback {
public:
	void set_target_filter(ZstFilter * filter);
	void run(ZstInputPlug * plug) override;
private:
	ZstFilter * m_filter;
};
