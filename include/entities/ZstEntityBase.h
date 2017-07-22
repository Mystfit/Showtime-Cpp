#pragma once

#include "ZstExports.h"
#include "Showtime.h"
#include "ZstPlug.h"
#include <vector>

class ZstEntityBase {
public:
	friend class ZstEndpoint;
	
	//Base entity
	ZST_EXPORT ZstEntityBase(ZstEntityBehaviour behaviour, const char * entity_type, const char * entity_name);
	ZST_EXPORT virtual void init();
	ZST_EXPORT ZstEntityBehaviour behaviour() const;
	ZST_EXPORT const char * entity_type() const;
	ZST_EXPORT const char * name() const;
	ZST_EXPORT int id() const;
	ZST_EXPORT bool is_registered();
	ZST_EXPORT ZstEntityBase * parent() const;

	//Patch
	ZST_EXPORT virtual const ZstEntityBase * get_child_entity_at(int index) const;
	ZST_EXPORT virtual const size_t num_children() const;
	ZST_EXPORT ZstPlug * get_plug_by_URI(const ZstURI uri);

	//Filter
	ZST_EXPORT virtual ZstInputPlug * create_input_plug(const char* name, ZstValueType val_type);
	ZST_EXPORT virtual ZstOutputPlug * create_output_plug(const char* name, ZstValueType val_type);
	ZST_EXPORT virtual void remove_plug(ZstPlug *plug);

protected:
	ZST_EXPORT virtual void compute(ZstInputPlug * plug);

private:
	//Base entity
	char * m_entity_type;
	char * m_name;
	int m_id;
	ZstEntityBehaviour m_behaviour;
	bool m_is_registered;
	ZstEntityBase * m_parent;

	//Patch
	std::vector<ZstEntityBase*> m_children;

	//Filter
	std::vector<ZstPlug*> m_plugs;
};