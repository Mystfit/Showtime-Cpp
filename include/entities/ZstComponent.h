#pragma once

#include <vector>
#include "ZstExports.h"
#include "ZstEntityBase.h"
#include "ZstCallbacks.h"
#include "ZstPlug.h"

#define COMPONENT_TYPE "filter"
class ZstComputeCallback;

class ZstComponent : public ZstEntityBase {
public:
	friend class ZstComputeCallback;
	ZST_EXPORT ZstComponent(const char * entity_type, const char * name);
	ZST_EXPORT ZstComponent(const char * entity_type, const char * name, ZstEntityBase * parent);
	ZST_EXPORT ~ZstComponent();
	ZST_EXPORT virtual void init() override;
	ZST_EXPORT virtual ZstPlug * get_plug_by_URI(const ZstURI & uri) const;
	ZST_EXPORT virtual void compute(ZstInputPlug * plug);

	ZST_EXPORT virtual ZstInputPlug * create_input_plug(const char* name, ZstValueType val_type);
	ZST_EXPORT virtual ZstOutputPlug * create_output_plug(const char* name, ZstValueType val_type);
	ZST_EXPORT virtual void remove_plug(ZstPlug *plug);

private:
	std::vector<ZstPlug*> m_plugs;
	ZstComputeCallback * m_compute_callback;
};
