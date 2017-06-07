#include "ZstURI.h"
using namespace std;

ZstURI::ZstURI() : m_direction(Direction::OUT_JACK)
{
	//Empty URI
}

ZstURI::ZstURI(const char * performer, const char * instrument, const char * name, const Direction direction) :
	m_performer(string(performer)),
	m_instrument(string(instrument)),
	m_name(string(name)),
	m_direction(direction) {
}

ZstURI::ZstURI(const ZstURI & copy) : 
	m_performer(copy.m_performer),
	m_instrument(copy.m_instrument),
	m_name(copy.m_name),
	m_direction(copy.m_direction)
{
}

ZstURI::~ZstURI()
{
}

const char * ZstURI::performer()
{
	return m_performer.c_str();
}

const char * ZstURI::instrument()
{
	return m_instrument.c_str();
}

const char * ZstURI::name()
{
	return m_name.c_str();
}

const ZstURI::Direction ZstURI::direction()
{
	return m_direction;
}

bool ZstURI::operator==(const ZstURI & other)
{
	return (m_performer == other.m_performer) &&
		(m_instrument == other.m_instrument) &&
		(m_name == other.m_name);
}

bool ZstURI::operator!=(const ZstURI & other)
{
	return !((m_performer == other.m_performer) &&
		(m_instrument == other.m_instrument) &&
		(m_name == other.m_name));
}

//bool ZstURI::operator<(const ZstURI& a, const ZstURI& b)
//{
//	
//}

const char * ZstURI::to_char() const {
	string out = m_performer + "/" + m_instrument + "/" + m_name + "/" + std::to_string(m_direction);
	char * result = new char[sizeof(out)];
	strcpy(result, out.c_str());
	return result;
}