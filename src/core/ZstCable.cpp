#include <msgpack.hpp>
#include <nlohmann/json.hpp>

#include "ZstCable.h"
#include "entities/ZstPlug.h"
#include "liasons/ZstPlugLiason.hpp"
#include "ZstEventDispatcher.hpp"

ZstCable::ZstCable() : 
	ZstSynchronisable(),
    m_address(),
	m_input(NULL),
	m_output(NULL)
{
}

ZstCable::ZstCable(const ZstCable & copy) : 
	ZstSynchronisable(),
    m_address(copy.m_address),
	m_input(copy.m_input),
	m_output(copy.m_output)
{
}

ZstCable::ZstCable(ZstInputPlug * input_plug, ZstOutputPlug * output_plug) :
	ZstSynchronisable(),
    m_address(input_plug->URI(), output_plug->URI()),
	m_input(input_plug),
	m_output(output_plug)
{
}

ZstCable::~ZstCable()
{
	m_input = NULL;
	m_output = NULL;
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
    if(!m_input || !m_output || !plugA || !plugB)
        return false;
	return is_attached(plugA->URI(), plugB->URI());
}

bool ZstCable::is_attached(ZstPlug * plug) const 
{
    if(!m_input || !m_output)
        return false;
	return (ZstURI::equal(m_input->URI(), plug->URI())) || (ZstURI::equal(m_output->URI(), plug->URI()));
}

void ZstCable::set_input(ZstInputPlug * input)
{
	m_input = input;
}

void ZstCable::set_output(ZstOutputPlug * output)
{
	m_output = output;
}

ZstInputPlug * ZstCable::get_input()
{
	return m_input;
}

ZstOutputPlug * ZstCable::get_output()
{
	return m_output;
}

const ZstCableAddress & ZstCable::get_address() const
{
    return m_address;
}


// -------------------------------
// Testing
// -------------------------------

void ZstCable::self_test()
{
	auto in = ZstURI("a/1");
	auto out = ZstURI("b/1");
    auto less = ZstURI("a/b");
    auto more = ZstURI("b/a");
    auto plug_in_A = std::make_unique<ZstInputPlug>(in.path(), ZST_INT);
    auto plug_out_A = std::make_unique<ZstOutputPlug>(out.path(), ZST_INT);
    auto plug_in_B = std::make_unique<ZstInputPlug>(less.path(), ZST_INT);
    auto plug_out_B = std::make_unique<ZstOutputPlug>(more.path(), ZST_INT);
    auto plug_in_C = std::make_unique<ZstInputPlug>(more.path(), ZST_INT);
    auto plug_out_C = std::make_unique<ZstOutputPlug>(less.path(), ZST_INT);

	auto cable_a = std::make_unique<ZstCable>(plug_in_A.get(), plug_out_A.get());
	assert(cable_a->get_address().get_input_URI() == in);
	assert(cable_a->get_address().get_output_URI() == out);
	assert(cable_a->is_attached(out));
	assert(cable_a->is_attached(in));
    
    //Test cable comparisons
	auto cable_b = std::make_unique<ZstCable>(plug_in_A.get(), plug_out_A.get());
    assert(ZstCableAddressEq{}(cable_a->get_address(), cable_b->get_address()));
	assert(ZstCableAddressHash{}(cable_a->get_address()) == ZstCableAddressHash{}(cable_b->get_address()));

    auto cable_c = std::make_unique<ZstCable>(plug_in_B.get(), plug_out_B.get());
    auto cable_d = std::make_unique<ZstCable>(plug_in_C.get(), plug_out_C.get());
    
    assert(ZstCableCompare{}(cable_c, cable_d));
    assert(ZstCableCompare{}(cable_c->get_address(), cable_d));
    assert(ZstCableCompare{}(cable_c, cable_d->get_address()));
    assert(!(ZstCableCompare{}(cable_d->get_address(), cable_c)));
    assert(!(ZstCableCompare{}(cable_d, cable_c->get_address())));

    //Test cable sets
    std::set<std::unique_ptr<ZstCable>, ZstCableCompare> cable_set;
    cable_set.insert(std::move(cable_c));
    cable_set.insert(std::move(cable_d));
    assert(cable_set.find(cable_c) != cable_set.end());
    assert(cable_set.find(cable_d) != cable_set.end());
}
