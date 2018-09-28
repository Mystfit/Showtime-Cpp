#pragma once

#include <set>
#include <ZstExports.h>
#include <entities/ZstEntityBase.h>
#include <adaptors/ZstFactoryAdaptor.hpp>

#define FACTORY_TYPE "fac"

//Forwards
template<typename T>
class ZstEventDispatcher;

//Typedefs
typedef std::shared_ptr<ZstEntityBase> ZstSharedEntity;

class ZstEntityFactory : public ZstEntityBase
{
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

	ZST_EXPORT virtual void write(std::stringstream & buffer) const override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;


	//Events

	ZST_EXPORT void add_adaptor(ZstFactoryAdaptor * adaptor);
	ZST_EXPORT void remove_adaptor(ZstFactoryAdaptor * adaptor);
	ZST_EXPORT ZstEventDispatcher<ZstFactoryAdaptor*> * factory_events();

protected:
	ZST_EXPORT virtual ZstEntityBase * activate_entity(ZstEntityBase * entity);
	ZST_EXPORT virtual void update_URI() override;
	ZST_EXPORT virtual void process_events();

private:
	void update_createable_URIs();
	std::set<ZstURI> m_creatables;
	ZstEventDispatcher<ZstFactoryAdaptor*> * m_factory_events;
};
