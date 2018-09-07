#include <exception>
#include <msgpack.hpp>
#include <ZstEntityFactory.h>

ZstEntityFactory::ZstEntityFactory(const char * name) : ZstEntityBase(name)
{
	set_entity_type(FACTORY_TYPE);
}

ZstEntityFactory::ZstEntityFactory(const ZstEntityFactory & other) : ZstEntityBase(other)
{
	m_creatables = other.m_creatables;
}

void ZstEntityFactory::add_creatable(const ZstURI & creatable_path)
{
	m_creatables.insert(creatable_path);
}

void ZstEntityFactory::remove_creatable(const ZstURI & creatable_path)
{
	try {
		m_creatables.erase(creatable_path);
	}
	catch (std::out_of_range) {}
}

void ZstEntityFactory::register_entity(ZstEntityBase * entity)
{
	//Activate entity and attach listeners here
	entity_events()->invoke([entity](ZstEntityAdaptor * adp) { adp->register_entity(entity); });
}

void ZstEntityFactory::write(std::stringstream & buffer) const
{	
	//Pack creatables
	ZstEntityBase::write(buffer);
	msgpack::pack(buffer, m_creatables.size());
	for (auto creatable : m_creatables) {
		msgpack::pack(buffer, creatable.path());
	}
}

void ZstEntityFactory::read(const char * buffer, size_t length, size_t & offset)
{
	//Unpack entity
	ZstEntityBase::read(buffer, length, offset);

	//Unpack creatable paths
	auto handle = msgpack::unpack(buffer, length, offset);
	int num_creatables = handle.get().via.i64;
	if (num_creatables > 0) {
		for (int i = 0; i < num_creatables; ++i) {
			handle = msgpack::unpack(buffer, length, offset);
			m_creatables.emplace(handle.get().via.str.ptr, handle.get().via.str.size);
		}
	}
}
