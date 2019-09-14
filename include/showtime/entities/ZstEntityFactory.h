#pragma once

#include <set>
#include <memory>

#include "ZstExports.h"
#include "entities/ZstEntityBase.h"
#include "adaptors/ZstFactoryAdaptor.hpp"

namespace showtime {

//Forwards
template<typename T>
class ZstEventDispatcher;

//Typedefs
typedef std::shared_ptr<ZstEntityBase> ZstSharedEntity;

class ZST_CLASS_EXPORTED ZstEntityFactory :
    public ZstEntityBase,
    virtual ZstSerialisable<Factory>
{
	using ZstEntityBase::add_adaptor;
	using ZstEntityBase::remove_adaptor;
	friend class ZstEntityFactoryLiason;
public:

	ZST_EXPORT ZstEntityFactory();
	ZST_EXPORT ZstEntityFactory(const char * name);
	ZST_EXPORT ZstEntityFactory(const ZstEntityFactory & other);
	ZST_EXPORT ~ZstEntityFactory();


	//Creatables

	ZST_EXPORT void add_creatable(const ZstURI & creatable_path);
	ZST_EXPORT void remove_creatable(const ZstURI & creatable_path);
	ZST_EXPORT ZstURIBundle & get_creatables(ZstURIBundle & bundle);
	ZST_EXPORT const ZstURI & get_creatable_at(size_t index);
	ZST_EXPORT size_t num_creatables();
	ZST_EXPORT void update_creatables();
	ZST_EXPORT void clear_creatables();
	ZST_EXPORT virtual ZstEntityBase * create_entity(const ZstURI & creatable_path, const char * name);

	
    //Serialisation
    
    using ZstEntityBase::serialize;
    using ZstEntityBase::deserialize;
    ZST_EXPORT void serialize(flatbuffers::Offset<Factory> & serialized_offset, flatbuffers::FlatBufferBuilder & buffer_builder) const override;
    ZST_EXPORT void deserialize(const Factory* buffer) override;
    
    
	//Events

	ZST_EXPORT void add_adaptor(std::shared_ptr<ZstFactoryAdaptor> adaptor);
	ZST_EXPORT void remove_adaptor(std::shared_ptr<ZstFactoryAdaptor> adaptor);
	ZST_EXPORT std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstFactoryAdaptor> > > & factory_events();

protected:
	ZST_EXPORT virtual ZstEntityBase * activate_entity(ZstEntityBase * entity);
	ZST_EXPORT virtual void update_URI() override;
	ZST_EXPORT virtual void process_events() override;

private:
	void update_createable_URIs();
	std::set<ZstURI> m_creatables;
	std::shared_ptr<ZstEventDispatcher<std::shared_ptr<ZstFactoryAdaptor> > > m_factory_events;
};

}
