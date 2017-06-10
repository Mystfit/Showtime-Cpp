#include "ZstURI.h"
using namespace std;


ZstURI::ZstURI() : m_direction(Direction::OUT_JACK)
{
	cout << "In ZSTURI default constructor" << endl;
	m_performer = "";
	m_instrument = "";
	m_name = "";
	m_direction = Direction::OUT_JACK;
}

ZstURI::ZstURI(string  performer, string  instrument, string  name, Direction direction) {
	m_performer = performer;
	m_instrument = instrument;
	m_name = name;
	m_direction = direction;
}


ZstURI::~ZstURI()
{
}

ZstURI * ZstURI::create(const char *  performer, const char *  instrument, const char *  name, Direction direction)
{
	return new ZstURI(string(performer), string(instrument), string(name), direction);
}

void ZstURI::destroy(ZstURI * uri) {
	delete uri;
}

string ZstURI::performer()
{
	return m_performer;
}

string ZstURI::instrument()
{
	return m_instrument;
}

string ZstURI::name(
){
	return m_name;
}

const ZstURI::Direction ZstURI::direction()
{
	return m_direction;
}

const char * ZstURI::performer_char() {
	return m_performer.c_str();
}

const char * ZstURI::instrument_char() {
	return m_instrument.c_str();
}

const char * ZstURI::name_char() {
	return m_name.c_str();
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

bool ZstURI::operator< (const ZstURI& b) const 
{
	return to_str() < b.to_str();
}

ZstURI ZstURI::from_str(const char * s)
{
	std::vector<std::string> tokens;
	Utils::str_split(string(s), tokens, "/");
	return ZstURI(tokens[0].c_str(), tokens[1].c_str(), tokens[2].c_str(), (ZstURI::Direction)std::atoi(tokens[3].c_str()));
}

const std::string ZstURI::to_str() const {
	return m_performer + "/" + m_instrument + "/" + m_name + "/" + std::to_string(m_direction);
}

