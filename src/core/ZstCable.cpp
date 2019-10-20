#include <msgpack.hpp>

#include "ZstCable.h"
#include "entities/ZstPlug.h"
#include "liasons/ZstPlugLiason.hpp"
#include "ZstEventDispatcher.hpp"

namespace showtime {

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

}
