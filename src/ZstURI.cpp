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

const char * ZstURI::path() const {
	return m_combined_path;
}

const size_t ZstURI::size() const
{
	return m_num_path_components;
}

ZstURI ZstURI::range(int start, int end) const
{
	if (start > size() || end > size()) {
		throw std::range_error("Start or end exceeds path length");
	}

	Str255 new_path;
	strcpy(new_path, segment(0));
	if (end - start > 1) {
		strcat(new_path, "/");
	}

	for (int i = start + 1; i < end; ++i) {
		strcat(new_path, segment(i));
		if (i < size() - 2) {
			strcat(new_path, "/");
		}
	}

	ZstURI new_uri = ZstURI(new_path);
	return new_uri;
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

const char * ZstURI::segment(int index) const
{
	if (index > size()-1 || index > MAX_PATH_LEN)
		throw std::range_error("Path index out of range");

	return m_path + m_path_offsets[index];
}

bool ZstURI::equal(const ZstURI & a, const ZstURI b)
{
	return (strcmp(a.path(), b.path()) == 0);
}

bool ZstURI::operator< (const ZstURI& b) const 
{
	return std::lexicographical_compare<const char*, const char*>(m_combined_path, m_combined_path + strlen(m_combined_path), b.m_combined_path, b.m_combined_path + strlen(b.m_combined_path));
}

bool ZstURI::is_empty() {
	return string(m_combined_path).empty();
}

ZstURI ZstURI::join(const ZstURI & a, const ZstURI & b)
{
	string a_parent = string(a.path());
	string combined_path = string(b.path());
	if(!a_parent.empty())
		combined_path.insert(0, a_parent + "/");

	return ZstURI(combined_path.c_str());
}

void ZstURI::build_split_path(const char * path)
{
	char * local_path = new char[255]();
	strcpy(local_path, path);
	char * token = strtok(local_path, "/");

	long offset = 0;
	int i = 0;
	while(token != NULL){
		m_path_offsets[i] = offset;
		size_t l = strlen(token);
		memcpy(m_path + offset, token, l);
		offset += l;
		m_path[offset] = 0;
		offset += 1;
		i++;

		token = strtok(NULL, "/");

		if (m_path_offsets[i] < 0) {
			throw std::range_error("Why is this less than 0?!");
		}
	}
	m_num_path_components = i;
	delete[] local_path;
}
