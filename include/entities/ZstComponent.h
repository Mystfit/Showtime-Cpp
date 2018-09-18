#pragma once

#include <vector>
#include <ZstExports.h>
#include <entities/ZstEntityBase.h>
#include <entities/ZstPlug.h>
#include <ZstCable.h>

#define COMPONENT_TYPE "cmp"

class ZstComponent : public ZstEntityBase {
public:
	ZST_EXPORT ZstComponent();
	ZST_EXPORT ZstComponent(const char * path);
	ZST_EXPORT ZstComponent(const char * component_type, const char * path);
	ZST_EXPORT ZstComponent(const ZstComponent & other);
	ZST_EXPORT virtual ~ZstComponent();
	
    //External factory function
	ZST_EXPORT virtual void create(const char * name, ZstEntityBase* parent) {};
    
    //Find a plug in this component by its URI
	ZST_EXPORT ZstPlug * get_plug_by_URI(const ZstURI & uri) const;

    //Overridable compute function that will process input plug events
	ZST_EXPORT virtual void compute(ZstInputPlug * plug) {};
        
    //Create and attach a new input plug to this component
	ZST_EXPORT ZstInputPlug * create_input_plug(const char* name, ZstValueType val_type);
    
    //Create and attach a new output plug to this component
	ZST_EXPORT ZstOutputPlug * create_output_plug(const char* name, ZstValueType val_type, bool reliable = true);

	//Transfer plug ownership to this component
	ZST_EXPORT int add_plug(ZstPlug * plug);
    
    //Remove a plug from this component
	ZST_EXPORT void remove_plug(ZstPlug *plug);

	//Hierarchy
	ZST_EXPORT virtual ZstCableBundle & get_child_cables(ZstCableBundle & bundle) const override;
	ZST_EXPORT virtual ZstEntityBundle & get_child_entities(ZstEntityBundle & bundle, bool include_parent = true) override;
    
	//Serialisation
	ZST_EXPORT virtual void write(std::stringstream & buffer) const override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;

	//Specific component type
	ZST_EXPORT const char * component_type() const;

protected:
	ZST_EXPORT void set_component_type(const char * component_type);
	ZST_EXPORT void set_component_type(const char * component_type, size_t len);

    //Set parent of this component
    ZST_EXPORT virtual void set_parent(ZstEntityBase * parent) override;
    
private:
	std::vector<ZstPlug*> m_plugs;
	char * m_component_type;
};
