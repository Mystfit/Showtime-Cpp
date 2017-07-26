#include <map>
#include "ZstURI.h"

using namespace std;

ZstURI::ZstURI() : 
	m_instrument(""),
	m_name(""),
	m_created_combined_char(false)
{
	build_combined_char();
}

ZstURI::ZstURI(const ZstURI &copy) : 
	m_instrument(""),
	m_name(""),
	m_created_combined_char(false)
{
	memcpy(m_instrument, copy.m_instrument, 255);
	memcpy(m_name, copy.m_name, 255);
	build_combined_char();
}

ZstURI::ZstURI(const char *  instrument, const char *  name) :
	m_instrument(""),
	m_name(""),
	m_created_combined_char(false)
{
	memcpy(m_instrument, instrument, 255);
	memcpy(m_name, name, 255);
	build_combined_char();
}

ZstURI::~ZstURI()
{
}

const string ZstURI::instrument() const{
	return string(m_instrument);
}

const string ZstURI::name() const {
	return string(m_name);
}

const char * ZstURI::instrument_char() {
	return m_instrument;
}

const char * ZstURI::name_char() {
	return m_name;
}

bool ZstURI::contains(ZstURI compare) {
	string original = string(m_combined_char);
	string comp = string(compare.to_char());
	bool result = false;
	size_t position = original.find(comp);

	//Found URI match at start of string
	if(position == 0){
		result = true;
	}
	else if (position > 0) {
		//Matched string not found at start of URI
		result = false;
	}
	else if (position == string::npos) {
		result = false;
	}
	return result;
}

bool ZstURI::operator==(const ZstURI & other)
{
	return (strcmp(m_instrument, other.m_instrument) == 0) &&
		(strcmp(m_name, other.m_name) == 0);
}

bool ZstURI::operator!=(const ZstURI & other)
{
	return !((strcmp(m_instrument, other.m_instrument) == 0) &&
		(strcmp(m_name, other.m_name) == 0));
}

bool ZstURI::operator< (const ZstURI& b) const 
{
	return std::lexicographical_compare<const char*, const char*>(m_combined_char, m_combined_char + strlen(m_combined_char), m_combined_char, m_combined_char + strlen(b.m_combined_char));
}

bool ZstURI::is_empty() {
	return instrument().empty() && name().empty();
}

ZstURI ZstURI::join(ZstURI a, ZstURI b)
{
	string a_parent = string(a.instrument_char());
	string b_parent = string(b.instrument_char());
	string combined_parent = a_parent + "/" + b_parent;
	return ZstURI(combined_parent.c_str(), b.name_char());
}

const char * ZstURI::to_char() const
{
	return m_combined_char;
}

ZstURI ZstURI::from_char(const char * s)
{
    string uri_str = string(s);
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
    
	return ZstURI(instrument.c_str(), plug.c_str());
}

void ZstURI::build_combined_char() {
	sprintf(m_combined_char, "%s/%s", m_instrument, m_name);
	m_created_combined_char = true;
}
