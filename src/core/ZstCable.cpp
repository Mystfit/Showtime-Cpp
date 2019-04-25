#include <msgpack.hpp>
#include <nlohmann/json.hpp>

#include "ZstCable.h"
#include "entities/ZstPlug.h"
#include "liasons/ZstPlugLiason.hpp"

ZstCable::ZstCable() : 
	ZstSynchronisable(),
    m_input_URI(""),
    m_output_URI(""),
	m_input(NULL),
	m_output(NULL)
{
	set_proxy();
}

ZstCable::ZstCable(const ZstCable & copy) : 
	ZstSynchronisable(),
    m_input_URI(copy.m_input_URI),
    m_output_URI(copy.m_output_URI),
	m_input(copy.m_input),
	m_output(copy.m_output)
{
}

ZstCable::ZstCable(const ZstURI & input_plug_URI, const ZstURI & output_plug_URI) :
	ZstSynchronisable(),
    m_input_URI(input_plug_URI),
    m_output_URI(output_plug_URI),
	m_input(NULL),
	m_output(NULL)
{
}

ZstCable::ZstCable(ZstInputPlug * input_plug, ZstOutputPlug * output_plug) :
	ZstSynchronisable(),
    m_input_URI(input_plug->URI()),
    m_output_URI(output_plug->URI()),
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

bool ZstCable::operator==(const ZstCable & other) const
{
	return (m_input_URI == other.m_input_URI) && (m_output_URI == other.m_output_URI);
}

bool ZstCable::operator!=(const ZstCable & other) const {
	return !(*this == other);
}

bool ZstCable::operator<(const ZstCable & other) const
{
    return std::tie(m_input_URI, m_output_URI) < std::tie(other.m_input_URI, other.m_output_URI);
}

bool ZstCable::is_attached(const ZstURI & uri) const
{
	return (ZstURI::equal(m_input_URI, uri)) || (ZstURI::equal(m_output_URI, uri));
}

bool ZstCable::is_attached(const ZstURI & uriA, const ZstURI & uriB) const
{
	return 	(ZstURI::equal(m_input_URI, uriA) || ZstURI::equal(m_input_URI, uriB)) &&
		(ZstURI::equal(m_output_URI, uriA) || ZstURI::equal(m_output_URI, uriB));
}

bool ZstCable::is_attached(ZstPlug * plugA, ZstPlug * plugB) const 
{
	return is_attached(plugA->URI(), plugB->URI());
}

bool ZstCable::is_attached(ZstPlug * plug) const 
{
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

const ZstURI & ZstCable::get_input_URI() const
{
	return m_input_URI;
}

const ZstURI & ZstCable::get_output_URI() const
{
	return m_output_URI;
}

void ZstCable::write_json(json & buffer) const
{
	buffer["output_uri"] = m_output_URI.path();
	buffer["input_uri"] = m_input_URI.path();
}

void ZstCable::read_json(const json & buffer)
{
	m_output_URI = ZstURI(buffer["output_uri"].get<std::string>().c_str(), buffer["output_uri"].get<std::string>().size());
	m_input_URI = ZstURI(buffer["input_uri"].get<std::string>().c_str(), buffer["input_uri"].get<std::string>().size());
}

size_t ZstCableHash::operator()(ZstCable* const& k) const
{
	std::size_t h1 = ZstURIHash{}(k->get_output_URI());
	std::size_t h2 = ZstURIHash{}(k->get_input_URI());
	return h1 ^ (h2 << 1);
}

bool ZstCableEq::operator()(ZstCable const * lhs, ZstCable const * rhs) const
{
	bool result = (*lhs == *rhs);
	return result;
}


// -------------------------------
// Testing
// -------------------------------

void ZstCable::self_test()
{
	ZstURI in = ZstURI("a/1");
	ZstURI out = ZstURI("b/1");
    ZstURI less = ZstURI("a/b");
    ZstURI more = ZstURI("b/a");

	ZstCable cable_a = ZstCable(in, out);
	assert(cable_a.get_input_URI() == in);
	assert(cable_a.get_output_URI() == out);
	assert(cable_a.is_attached(out));
	assert(cable_a.is_attached(in));

    //Test cable comparisons
	ZstCable cable_b = ZstCable(in, out);
	assert(cable_b == cable_a);
	assert(ZstCableEq{}(&cable_a, &cable_b));
	assert(ZstCableHash{}(&cable_a) == ZstCableHash{}(&cable_b));
    
    ZstCable cable_c = ZstCable(less, more);
    ZstCable cable_d = ZstCable(more, less);
    assert(cable_c < cable_d);
    assert(!(cable_d < cable_c));
    assert(!(cable_c < cable_c));
    
    //Test cable sets
    std::set<ZstCable> cable_set;
    cable_set.insert(cable_c);
    cable_set.insert(cable_d);
    assert(cable_set.find(cable_c) != cable_set.end());
    assert(cable_set.find(cable_d) != cable_set.end());

	//Test cable going out of scope
	{
		ZstCable cable_c = ZstCable(ZstURI("foo"), ZstURI("bar"));
		assert(cable_c != cable_a);
	}
}
