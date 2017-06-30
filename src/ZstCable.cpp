#include "ZstCable.h"

ZstCable::ZstCable()
{
}

ZstCable::ZstCable(const ZstCable & copy) : 
	m_input(copy.m_input),
	m_output(copy.m_output)
{
}

ZstCable::ZstCable(const ZstEvent & e)
{
	m_input = e.get_first();
	m_output = e.get_second();
}

ZstCable::ZstCable(const ZstURI output, const ZstURI input) :
	m_output(output),
	m_input(input)
{
}

ZstCable::~ZstCable()
{
}

bool ZstCable::operator==(const ZstURI & other)
{
	return (m_input == other) || (m_output == other);
}

bool ZstCable::operator==(const ZstCable & other)
{
	return (m_input == other.m_input) || (m_output == other.m_output);
}

bool ZstCable::operator!=(const ZstURI & other)
{
	return 	!((m_input == other) || (m_output == other));
}

ZstURI & ZstCable::get_input()
{
	return m_input;
}

ZstURI & ZstCable::get_output()
{
	return m_output;
}
