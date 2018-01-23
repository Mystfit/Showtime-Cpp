#include "ZstCable.h"
#include "entities/ZstPlug.h"
#include "msgpack.hpp"

ZstCable::ZstCable() : 
	m_input(NULL),
	m_output(NULL),
	m_input_URI(""),
	m_output_URI("")
{
}

ZstCable::ZstCable(const ZstCable & copy) : 
	m_input(copy.m_input),
	m_output(copy.m_output),
	m_input_URI(copy.m_input_URI),
	m_output_URI(copy.m_output_URI)
{
}

ZstCable::ZstCable(ZstPlug * input_plug, ZstPlug * output_plug) :
	m_input(input_plug),
	m_output(output_plug),
	m_input_URI(input_plug->URI()),
	m_output_URI(output_plug->URI())
{
}

ZstCable::~ZstCable()
{
}

bool ZstCable::operator==(const ZstCable & other)
{
	return ZstURI::equal(m_input_URI, other.m_input_URI) && ZstURI::equal(m_output_URI, other.m_output_URI);
}

bool ZstCable::operator!=(const ZstCable & other)
{
	return !(ZstURI::equal(m_input_URI, other.m_input_URI) && ZstURI::equal(m_output_URI, other.m_output_URI));
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

bool ZstCable::is_activated()
{
	return m_is_activated;
}

bool ZstCable::is_attached(ZstPlug * plug) const 
{
	return (ZstURI::equal(m_input->URI(), plug->URI())) || (ZstURI::equal(m_output->URI(), plug->URI()));
}

ZstPlug * ZstCable::get_input()
{
	return m_input;
}

ZstPlug * ZstCable::get_output()
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

void ZstCable::unplug()
{
	get_input()->remove_cable(this);
	get_output()->remove_cable(this);
}

void ZstCable::write(std::stringstream & buffer)
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

void ZstCable::set_activated()
{
	m_is_activated = true;
}

ZstCableBundle::ZstCableBundle()
{
}

ZstCableBundle::~ZstCableBundle()
{
}

void ZstCableBundle::add(ZstCable * cable)
{
	m_cables.push_back(cable);
}

ZstCable * ZstCableBundle::cable_at(size_t index)
{
	return m_cables[index];
}

size_t ZstCableBundle::size()
{
	return m_cables.size();
}
