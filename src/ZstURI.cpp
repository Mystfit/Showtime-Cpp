#include "ZstURI.h"
using namespace std;

ZstURI::ZstURI(std::string performer, std::string name, std::string instrument, Direction direction) :
	m_performer(performer),
	m_instrument(instrument),
	m_name(name),
	m_direction(direction) {
}

ZstURI::~ZstURI()
{
}

std::string ZstURI::performer() const
{
	return m_performer;
}

std::string ZstURI::instrument() const
{
	return m_instrument;
}

std::string ZstURI::name() const
{
	return m_name;
}

ZstURI::Direction ZstURI::direction() const
{
	return m_direction;
}

bool ZstURI::operator==(const ZstURI & other) const
{
	return (m_performer == other.performer()) &&
		(m_instrument == other.instrument()) &&
		(m_name == other.name());
}

bool ZstURI::operator<(const ZstURI & a) const
{
	{ return to_str() < a.to_str(); }
}

string ZstURI::to_str() const{
	return m_performer + "/" + m_instrument + "/" + m_name + "/" + std::to_string(m_direction);
}