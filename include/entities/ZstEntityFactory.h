#pragma once

#include <unordered_set>
#include <ZstExports.h>
#include <entities/ZstEntityBase.h>
#include <adaptors/ZstEntityAdaptor.hpp>

#define FACTORY_TYPE "fac"

typedef std::shared_ptr<ZstEntityBase> ZstSharedEntity;

class ZstEntityFactory : public ZstEntityBase, private ZstEntityAdaptor
{
	friend class ZstEntityFactoryLiason;
public:
	ZST_EXPORT ZstEntityFactory();
	ZST_EXPORT ZstEntityFactory(const char * name);
	ZST_EXPORT ZstEntityFactory(const ZstEntityFactory & other);


	//Creatables

	ZST_EXPORT void add_creatable(const ZstURI & creatable_path);
	ZST_EXPORT void remove_creatable(const ZstURI & creatable_path);
	ZST_EXPORT void get_creatables(ZstURIBundle & bundle);
	ZST_EXPORT virtual ZstEntityBase * create_entity(const ZstURI & creatable_path, const char * name);

	
	//Serialisation

	ZST_EXPORT virtual void write(std::stringstream & buffer) const override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;

protected:
	ZST_EXPORT virtual ZstEntityBase * activate_entity(ZstEntityBase * entity);
	ZST_EXPORT virtual void update_URI() override;

private:
	std::unordered_set<ZstURI, ZstURIHash> m_creatables;
};
