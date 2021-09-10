#include <showtime/ZstCable.h>
#include <showtime/entities/ZstPlug.h>
#include "liasons/ZstPlugLiason.hpp"
#include "ZstEventDispatcher.hpp"

namespace showtime {

ZstCable::ZstCable() : 
	ZstSynchronisable(),
	m_hierarchy_events(std::make_shared< ZstEventDispatcher<ZstHierarchyAdaptor> >()),
    m_address()
{
}

ZstCable::ZstCable(const ZstCable & copy) : 
	ZstSynchronisable(),
	m_hierarchy_events(std::make_shared< ZstEventDispatcher<ZstHierarchyAdaptor> >()),
    m_address(copy.m_address)
{
}

ZstCable::ZstCable(ZstInputPlug * input_plug, ZstOutputPlug * output_plug) :
	ZstSynchronisable(),
	m_hierarchy_events(std::make_shared< ZstEventDispatcher<ZstHierarchyAdaptor> >()),
    m_address(input_plug->URI(), output_plug->URI())
{
}

ZstCable::~ZstCable()
{
	m_hierarchy_events->remove_all_adaptors();
}

void ZstCable::disconnect()
{
	this->enqueue_deactivation();
}

bool ZstCable::is_attached(const ZstURI & uri) const
{
	return (ZstURI::equal(m_address.get_input_URI(), uri)) || (ZstURI::equal(m_address.get_output_URI(), uri));
}

bool ZstCable::is_attached(const ZstURI & uriA, const ZstURI & uriB) const
{
	return 	(ZstURI::equal(m_address.get_input_URI(), uriA) || ZstURI::equal(m_address.get_input_URI(), uriB)) &&
		(ZstURI::equal(m_address.get_output_URI(), uriA) || ZstURI::equal(m_address.get_output_URI(), uriB));
}

bool ZstCable::is_attached(ZstPlug * plugA, ZstPlug * plugB) const 
{
    if(!plugA || !plugB)
        return false;
	return is_attached(plugA->URI(), plugB->URI());
}

bool ZstCable::is_attached(ZstPlug * plug) const 
{
	auto input_plug = get_input();
	auto output_plug = get_output();

    if(!input_plug || !output_plug)
        return false;
	return (ZstURI::equal(input_plug->URI(), plug->URI())) || (ZstURI::equal(output_plug->URI(), plug->URI()));
}

ZstInputPlug * ZstCable::get_input() const
{
	ZstInputPlug* out_input_plug = nullptr;

	// Lookup entity in hierarchy so we don't have to hold onto an entity pointer
	m_hierarchy_events->invoke([this, &out_input_plug](ZstHierarchyAdaptor* adp) {
		auto entity = adp->find_entity(this->get_address().get_input_URI());
		if (!entity)
			return;

		if (entity->entity_type() == ZstEntityType::PLUG) {
			auto plug = static_cast<ZstPlug*>(entity);
			if (plug->direction() == ZstPlugDirection::IN_JACK) {
				auto input_plug = static_cast<ZstInputPlug*>(plug);
				out_input_plug = input_plug;
			}
		}
	});

	return out_input_plug;
}

ZstOutputPlug * ZstCable::get_output() const
{
	ZstOutputPlug* out_output_plug = nullptr;

	// Lookup entity in hierarchy so we don't have to hold onto an entity pointer
	m_hierarchy_events->invoke([this, &out_output_plug](ZstHierarchyAdaptor* adp) {
		auto entity = adp->find_entity(this->get_address().get_output_URI());
		if (!entity)
			return;

		if (entity->entity_type() == ZstEntityType::PLUG) {
			auto plug = static_cast<ZstPlug*>(entity);
			if (plug->direction() == ZstPlugDirection::OUT_JACK) {
				auto output_plug = static_cast<ZstOutputPlug*>(plug);
				out_output_plug = output_plug;
			}
		}
	});

	return out_output_plug;
}

const ZstCableAddress & ZstCable::get_address() const
{
    return m_address;
}

void ZstCable::add_adaptor(std::shared_ptr<ZstHierarchyAdaptor> adaptor)
{
	m_hierarchy_events->add_adaptor(adaptor);
}

void ZstCable::remove_adaptor(std::shared_ptr<ZstHierarchyAdaptor> adaptor)
{
	m_hierarchy_events->remove_adaptor(adaptor);
}

}
