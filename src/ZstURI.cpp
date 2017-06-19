#include "ZstURI.h"
using namespace std;


ZstURI::ZstURI() : m_direction(Direction::OUT_JACK)
{
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

const std::string ZstURI::to_str() const {
	return m_performer + "/" + m_instrument + "/" + m_name + "?d=" + std::to_string(m_direction);
}

const char * ZstURI::to_char() const
{
	return to_str().c_str();
}

ZstURI ZstURI::from_str(const std::string s)
{
	std::vector<std::string> uri_args_split;
    
	Utils::str_split(string(s), uri_args_split, "?");
    
    string uri_str = uri_args_split[0];
    string args_str = uri_args_split[1];
    
    std::vector<std::string> uri;
    Utils::str_split(uri_str, uri, "/");
    
    string performer = uri[0];
    
    string instrument = "";
    for(int i = 1; i < uri.size()-1; ++i){
        instrument += uri[i];
        if(i < uri.size() - 2)
            instrument += "/";
    }
    
    string plug = uri[uri.size()-1];
    
    std::vector<std::string> args_split;
    Utils::str_split(args_str, args_split, "&");
    
    std::map<string, string> arg_map;
    for(int i = 0; i < args_split.size(); ++i){
        std::vector<std::string> single_arg_split;
        Utils::str_split(args_split[i], single_arg_split, "=");
        arg_map[single_arg_split[0]] = single_arg_split[1];
    }

    ZstURI::Direction dir = (ZstURI::Direction)std::atoi(arg_map["d"].c_str());
	return ZstURI(performer.c_str(), instrument.c_str(), plug.c_str(), dir);
}

ZstURI ZstURI::from_char(const char * s)
{
	return from_str(string(s));
}

