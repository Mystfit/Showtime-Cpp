#include "ZstCable.h"

ZstCable::ZstCable()
{
}

ZstCable::ZstCable(const ZstCable & copy) : 
	m_input(copy.m_input),
	m_output(copy.m_output)
{
}

ZstCable::ZstCable(const ZstEvent & e) :
	m_input(e.get_first()),
	m_output(e.get_second())
{
}

ZstCable::ZstCable(const ZstURI output, const ZstURI input) :
	m_output(output),
	m_input(input)
{
}

ZstCable::~ZstCable()
{
}

bool ZstCable::operator==(const ZstCable & other)
{
	return (m_input == other.m_input) && (m_output == other.m_output);
}

bool ZstCable::operator!=(const ZstCable & other)
{
	return !((m_input == other.m_input) && (m_output == other.m_output));
}

bool ZstCable::is_attached(const ZstURI & uri)
{
	return (m_input == uri) || (m_output == uri);
}

bool ZstCable::is_attached(const ZstURI & uriA, const ZstURI & uriB)
{
	return (m_input == uriA || m_input == uriB) && (m_output == uriA || m_output == uriB);
}

ZstURI & ZstCable::get_input()
{
	return m_input;
}

ZstURI & ZstCable::get_output()
{
	return m_output;
}
