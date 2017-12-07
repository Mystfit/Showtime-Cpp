#include "ZstCable.h"
#include "msgpack.hpp"

ZstCable::ZstCable()
{
}

ZstCable::ZstCable(const ZstCable & copy) : 
	m_input(copy.m_input),
	m_output(copy.m_output)
{
}

ZstCable::ZstCable(const ZstURI output, const ZstURI input) :
	m_input(input),
    m_output(output)
{
}

ZstCable::~ZstCable()
{
}

bool ZstCable::operator==(const ZstCable & other)
{
	return ZstURI::equal(m_input, other.m_input) && ZstURI::equal(m_output, other.m_output);
}

bool ZstCable::operator!=(const ZstCable & other)
{
	return !(ZstURI::equal(m_input, other.m_input) && ZstURI::equal(m_output, other.m_output));
}

bool ZstCable::is_attached(const ZstURI & uri)
{
	return (ZstURI::equal(m_input, uri)) || (ZstURI::equal(m_output, uri));
}

bool ZstCable::is_attached(const ZstURI & uriA, const ZstURI & uriB)
{
	return 	(ZstURI::equal(m_input, uriA) || ZstURI::equal(m_input, uriB)) &&
		(ZstURI::equal(m_output, uriA) || ZstURI::equal(m_output, uriB));
}

ZstURI & ZstCable::get_input()
{
	return m_input;
}

ZstURI & ZstCable::get_output()
{
	return m_output;
}

void ZstCable::write(std::stringstream & buffer)
{
	get_output().write(buffer);
	get_input().write(buffer);
}

void ZstCable::read(const char * buffer, size_t length, size_t & offset)
{
	m_output.read(buffer, length, offset);
	m_input.read(buffer, length, offset);
}