#pragma once

#include <unordered_set>
#include <ZstExports.h>
#include <entities/ZstEntityBase.h>
#include <ZstEventDispatcher.hpp>

#define FACTORY_TYPE "fac"

class ZstEntityFactory : public ZstEntityBase
{
public:
	ZST_EXPORT ZstEntityFactory(const char * name);
	ZST_EXPORT ZstEntityFactory(const ZstEntityFactory & other);

	ZST_EXPORT void add_creatable(const ZstURI & creatable_path);
	ZST_EXPORT void remove_creatable(const ZstURI & creatable_path);
	ZST_EXPORT virtual std::shared_ptr<ZstEntityBase> create_entity(const ZstURI & creatable_path, const char * name) = 0;

	ZST_EXPORT virtual void write(std::stringstream & buffer) const override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;
protected:
	ZST_EXPORT virtual void register_entity(ZstEntityBase * entity);

private:
	std::unordered_set<ZstURI, ZstURIHash> m_creatables;
};
