#include <msgpack.hpp>
#include <ZstCable.h>
#include <entities/ZstPlug.h>
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

ZstCable * ZstCable::create(const ZstURI & input, const ZstURI & output)
{
	return new ZstCable(input, output);
}

ZstCable * ZstCable::create(ZstInputPlug * input, ZstOutputPlug * output)
{
	return new ZstCable(input, output);
}

void ZstCable::destroy(ZstCable * cable)
{
	delete cable;
}

void ZstCable::disconnect()
{
	this->enqueue_deactivation();
}

bool ZstCable::operator==(const ZstCable & other) const
{
	return (m_input_URI == other.m_input_URI) && (m_output_URI == other.m_output_URI);
}

bool ZstCable::operator!=(const ZstCable & other)
{
	return !(*this == other);
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

void ZstCable::write(std::stringstream & buffer) const
{
	msgpack::pack(buffer, m_output_URI.path());
	msgpack::pack(buffer, m_input_URI.path());
}

void ZstCable::read(const char * buffer, size_t length, size_t & offset)
{
	auto handle = msgpack::unpack(buffer, length, offset);
	m_output_URI = ZstURI(handle.get().via.str.ptr, handle.get().via.str.size);

	handle = msgpack::unpack(buffer, length, offset);
	m_input_URI = ZstURI(handle.get().via.str.ptr, handle.get().via.str.size);
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

	ZstCable cable_a = ZstCable(in, out);
	assert(cable_a.get_input_URI() == in);
	assert(cable_a.get_output_URI() == out);
	assert(cable_a.is_attached(out));
	assert(cable_a.is_attached(in));

	ZstCable cable_b = ZstCable(in, out);
	assert(cable_b == cable_a);
	assert(ZstCableEq{}(&cable_a, &cable_b));
	assert(ZstCableHash{}(&cable_a) == ZstCableHash{}(&cable_b));

	//Test cable going out of scope
	{
		ZstCable cable_c = ZstCable(ZstURI("foo"), ZstURI("bar"));
		assert(cable_c != cable_a);
	}
}
