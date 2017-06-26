#include <map>
#include "ZstURI.h"

using namespace std;

ZstURI::ZstURI() : 
	m_performer(""),
	m_instrument(""),
	m_name(""),
	m_direction(Direction::NONE)
{
}

ZstURI::ZstURI(const char *  performer, const char *  instrument, const char *  name, Direction direction){
	
	int performer_size = strlen(performer);
	int instrument_size = strlen(instrument);
	int name_size = strlen(name);

	m_performer = new char[performer_size + 1]();
	m_instrument = new char[instrument_size + 1]();
	m_name = new char[name_size + 1]();

	strncpy(m_performer, performer, performer_size);
	strncpy(m_instrument, instrument, instrument_size);
	strncpy(m_name, name, name_size);

	m_direction = direction;
}

ZstURI::~ZstURI()
{
}

ZstURI * ZstURI::create(const char *  performer, const char *  instrument, const char *  name, Direction direction)
{
	int performer_size = strlen(performer);
	int instrument_size = strlen(instrument);
	int name_size = strlen(name);


	char * perf_copy = new char[performer_size + 1]();
	char * instrument_copy = new char[instrument_size + 1]();
	char * name_copy = new char[name_size + 1]();

	strncpy(perf_copy, performer, performer_size);
	strncpy(instrument_copy, instrument, instrument_size);
	strncpy(name_copy, name, name_size);

	return new ZstURI(perf_copy, instrument_copy, name_copy, direction);
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
	return to_str() < b.to_str();
}

bool ZstURI::is_empty() {
	return performer().empty() && instrument().empty() && name().empty() && m_direction == Direction::NONE;
}

const std::string ZstURI::to_str() const {
	return string(string(m_performer) + "/" + string(m_instrument) + "/" + string(m_name) + "?d=" + std::to_string(m_direction));
}

const char * ZstURI::to_char() const
{
	return to_str().c_str();
}

ZstURI ZstURI::from_str(const char * s)
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

