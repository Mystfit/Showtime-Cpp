#include <map>
#include "ZstURI.h"

using namespace std;

ZstURI::ZstURI() : 
	m_performer(""),
	m_instrument(""),
	m_name(""),
	m_direction(Direction::NONE),
	m_created_combined_char(false)
{
	build_combined_char();
}

ZstURI::ZstURI(const ZstURI &copy) : 
	m_performer(""),
	m_instrument(""),
	m_name(""),
	m_direction(Direction::NONE),
	m_created_combined_char(false)
{
	memcpy(m_performer, copy.m_performer, 255);
	memcpy(m_instrument, copy.m_instrument, 255);
	memcpy(m_name, copy.m_name, 255);
	m_direction = copy.m_direction;
	build_combined_char();
}

ZstURI::ZstURI(const char *  performer, const char *  instrument, const char *  name, Direction direction) :
	m_performer(""),
	m_instrument(""),
	m_name(""),
	m_direction(Direction::NONE),
	m_created_combined_char(false)
{
	memcpy(m_performer, performer, 255);
	memcpy(m_instrument, instrument, 255);
	memcpy(m_name, name, 255);
	m_direction = direction;
	build_combined_char();
}

ZstURI::~ZstURI()
{
}

ZstURI * ZstURI::create(const char *  performer, const char *  instrument, const char *  name, Direction direction)
{
	return new ZstURI(performer, instrument, name, direction);
}

ZstURI * ZstURI::create_empty() {
	return new ZstURI();
}

void ZstURI::destroy(ZstURI * uri) {
	delete uri;
}

const string ZstURI::performer() const{
	return string(m_performer);
}

const string ZstURI::instrument() const{
	return string(m_instrument);
}

const string ZstURI::name() const {
	return string(m_name);
}

const ZstURI::Direction ZstURI::direction()
{
	return m_direction;
}

const char * ZstURI::performer_char() {
	return m_performer;
}

const char * ZstURI::instrument_char() {
	return m_instrument;
}

const char * ZstURI::name_char() {
	return m_name;
}

bool ZstURI::operator==(const ZstURI & other)
{
	return (strcmp(m_performer, other.m_performer) == 0) &&
		(strcmp(m_instrument, other.m_instrument) == 0) &&
		(strcmp(m_name, other.m_name) == 0);
}

bool ZstURI::operator!=(const ZstURI & other)
{
	return !((strcmp(m_performer, other.m_performer) == 0) &&
		(strcmp(m_instrument, other.m_instrument) == 0) &&
		(strcmp(m_name, other.m_name) == 0));
}

bool ZstURI::operator< (const ZstURI& b) const 
{
	return std::lexicographical_compare<const char*, const char*>(m_combined_char, m_combined_char + strlen(m_combined_char), m_combined_char, m_combined_char + strlen(b.m_combined_char));
}

bool ZstURI::is_empty() {
	return performer().empty() && instrument().empty() && name().empty() && m_direction == Direction::NONE;
}


const char * ZstURI::to_char() const
{
	return m_combined_char;
}

ZstURI ZstURI::from_char(const char * s)
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
    
    //Remove input char
    delete s;
	return ZstURI(performer.c_str(), instrument.c_str(), plug.c_str(), dir);
}

void ZstURI::build_combined_char() {
	sprintf(m_combined_char, "%s/%s/%s?d=%i", m_performer, m_instrument, m_name, (int)m_direction);
	m_created_combined_char = true;
}
