#include <map>
#include "ZstURI.h"
#include "ZstUtils.hpp"

using namespace std;

ZstURI::ZstURI() : 
	m_path(""),
	m_combined_path(""),
	m_path_offsets{ 0 },
	m_num_path_components(0)
{
	build_split_path("");
}

ZstURI::ZstURI(const ZstURI &copy) : 
	m_path(""),
	m_combined_path(""),
	m_path_offsets{ 0 },
	m_num_path_components(0)
{
	memcpy(m_combined_path, copy.m_combined_path, 255);
	build_split_path(copy.m_combined_path);
}

ZstURI::ZstURI(const char *  path) :
	m_path(""),
	m_combined_path(""),
	m_path_offsets{ 0 },
	m_num_path_components(0)
{
	memcpy(m_combined_path, path, 255);
	build_split_path(path);
}

ZstURI::~ZstURI()
{
}

const char * ZstURI::path() {
	return m_combined_path;
}

const size_t ZstURI::size() const
{
	return m_num_path_components;
}

ZstURI ZstURI::range(int start, int end) const
{
	if (start > m_num_path_components || end > m_num_path_components) {
		throw std::range_error("Start or end exceeds path length");
	}

	string new_path = "";
	const char * path_ptr;
	for (int i = start; i < end; ++i) {
		string s = string(get(i));
		new_path += s;
		if (i < size() - 2) {
			new_path += "/";
		}
	}

	//Return copy of new path as a URI
	return ZstURI(new_path.c_str());
}

bool ZstURI::contains(ZstURI compare) {
	string original = string(m_combined_path);
	string comp = string(compare.path());
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

const char * ZstURI::get(int index) const
{
	if (index > m_num_path_components-1 || index > MAX_PATH_LEN)
		throw std::range_error("Path index out of range");

	return m_path + m_path_offsets[index];
}

const char * ZstURI::operator[](int index)
{
	return get(index);
}

bool ZstURI::operator==(const ZstURI & other)
{
	return (strcmp(m_combined_path, other.m_combined_path) == 0);
}

bool ZstURI::operator!=(const ZstURI & other)
{
	return !((strcmp(m_combined_path, other.m_combined_path) == 0));
}

bool ZstURI::operator< (const ZstURI& b) const 
{
	return std::lexicographical_compare<const char*, const char*>(m_combined_path, m_combined_path + strlen(m_combined_path), b.m_combined_path, b.m_combined_path + strlen(b.m_combined_path));
}

bool ZstURI::is_empty() {
	return string(m_combined_path).empty();
}

ZstURI ZstURI::join(ZstURI a, ZstURI b)
{
	string a_parent = string(a.path());
	string combined_path = string(b.path());
	if(!a_parent.empty())
		combined_path.insert(0, a_parent + "/");

	return ZstURI(combined_path.c_str());
}

ZstURI ZstURI::from_char(const char * s)
{
    string uri_str = string(s);
    std::vector<std::string> uri;
    Utils::str_split(uri_str, uri, "/");
        
    string path = "";
    for(int i = 0; i < uri.size(); ++i){
        path += uri[i];
        if(i < uri.size() - 1)
            path += "/";
    }
        
	return ZstURI(path.c_str());
}

void ZstURI::build_split_path(const char * path)
{
	string p = string(path);
	vector<string> tokens;
	Utils::str_split(p, tokens, "/");
	m_num_path_components = tokens.size();

	long offset = 0;
 	for (int i = 0; i < tokens.size(); ++i) {
		string s = tokens[i];	
		m_path_offsets[i] = offset;
		memcpy(m_path + offset, s.c_str(), s.size());
		offset += s.size();
		m_path[offset] = 0;
		offset += 1;

		if (m_path_offsets[i] < 0) {
			throw std::range_error("WHy is this less than 0?!");
		}
	}
}
