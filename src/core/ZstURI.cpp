#include <exception>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <showtime/ZstURI.h>

namespace showtime {

ZstURI::ZstURI() : m_component_count(0)
{
	m_original_path = create_pstr("");
	m_component_count = 0;
}


ZstURI::~ZstURI() {
	free(m_original_path.cstr);
	for (size_t i = 0; i < m_component_count; ++i) {
		free(m_components[i].cstr);
	}
}

ZstURI::ZstURI(const ZstURI& copy) :
	m_component_count(0)
{
	m_original_path = create_pstr(copy.m_original_path.cstr, copy.m_original_path.length);
	init();
}

ZstURI& ZstURI::operator=(const ZstURI& other)
{
	m_original_path = create_pstr(other.m_original_path.cstr, other.m_original_path.length);
	init();
	return *this;
}

ZstURI& ZstURI::operator=(ZstURI&& source)
{
	//Free original uri memory
	free(m_original_path.cstr);
	for (size_t i = 0; i < m_component_count; ++i) {
		free(m_components[i].cstr);
	}

	//Move source uri memory contents over
	m_original_path = std::move(source.m_original_path);
	m_component_count = source.m_component_count;
	for (size_t i = 0; i < source.m_component_count; ++i) {
		m_components[i] = std::move(source.m_components[i]);
	}

	//Reset original URI
	source.m_component_count = 0;
	source.m_original_path = pstr();
	for (size_t i = 0; i < source.m_component_count; ++i) {
		m_components[i] = pstr();
	}

	return *this;
}

//

ZstURI::ZstURI(const char* path)
{
	if (path)
		m_original_path = create_pstr(path);
	else
		m_original_path = create_pstr("");

	init();
}

ZstURI::ZstURI(const char* path, size_t len)
{
	if (path)
		m_original_path = create_pstr(path, len);
	else
		m_original_path = create_pstr("");

	init();
}

void ZstURI::init()
{
	m_component_count = 0;

	if (m_original_path.cstr[0] == 0)
		return;

	size_t len = 0;
	size_t word_offset = 0;
	for (size_t i = 0; i < m_original_path.length; i++) {
		if (m_original_path.cstr[i] == DELIM) {
			m_components[m_component_count].length = len;
			m_components[m_component_count].cstr = (char*)malloc(len + 1);
			memcpy(m_components[m_component_count].cstr, &m_original_path.cstr[word_offset], len);
			m_components[m_component_count].cstr[len] = '\0';
			word_offset += len + 1;
			m_component_count++;
			len = 0;
		}
		else {
			len++;
		}
	}

	//Add last component
	m_components[m_component_count].length = len;		//Don't include trailing 0 in component length
	m_components[m_component_count].cstr = (char*)malloc(len + 1);
	memcpy(m_components[m_component_count].cstr, &m_original_path.cstr[word_offset], len);
	m_components[m_component_count].cstr[len] = '\0';
	m_component_count++;
}

//
const char* ZstURI::path() const
{
	return m_original_path.cstr;
}

const char* ZstURI::segment(size_t index) const
{
	if (index >= m_component_count) {
		throw std::range_error("Start or end index out of range of path components");
	}
	return m_components[index].cstr;
}

const size_t ZstURI::size() const
{
	return m_component_count;
}

const size_t ZstURI::full_size() const
{
	return strlen(m_original_path.cstr);
}

ZstURI ZstURI::operator+(const ZstURI& other) const
{
	size_t new_length = m_original_path.length + other.m_original_path.length + 2;

	char* new_path = (char*)malloc(new_length + 1);
	memcpy(new_path, m_original_path.cstr, m_original_path.length);
	new_path[m_original_path.length] = '\0';

	strncat(new_path, "/", 1);
	strncat(new_path, other.m_original_path.cstr, other.m_original_path.length);
	new_path[new_length - 1] = '\0';

	ZstURI result = ZstURI(new_path);
	free(new_path);
	new_path = 0;
	return result;
}

ZstURI ZstURI::range(size_t start, size_t end) const
{
	if ((end - start) > size())
		throw std::range_error("Start or end exceeds path length");

	size_t index = 0;
	size_t start_position = 0;

	for (; index < start; index++)
		start_position += m_components[index].length + 1;

	size_t length = 0;
	for (index = start; index <= end; index++)
		length += m_components[index].length;

	//Add seperators to length
	length += end - start;

	char* start_s = &m_original_path.cstr[start_position];
	return ZstURI(start_s, length);
}

ZstURI ZstURI::parent() const
{
	int s = static_cast<int>(this->size());
	int parent_index = s - 2;
	if (parent_index < 0) {
		return ZstURI();
	}
	return range(0, parent_index);
}

ZstURI ZstURI::first() const
{
	if (size() < 1)
		throw std::out_of_range("URI is empty");
	return ZstURI(m_components[0].cstr);
}

ZstURI ZstURI::last() const
{
	if (size() < 1)
		throw std::out_of_range("URI is empty");
	return ZstURI(m_components[size() - 1].cstr);
}

bool ZstURI::contains(const ZstURI& compare) const
{
	if (compare.size() > size()) {
		return false;
	}

	int s = static_cast<int>(size());
	int other_s = static_cast<int>(compare.size());
	int shortest = std::min(other_s, s);
	int contiguous = 0;

	for (int i = 0; i < shortest; ++i) {
		if (i < other_s) {
			if (strcmp(compare.m_components[i].cstr, this->m_components[i].cstr) == 0) {
				if (++contiguous == shortest)
					return true;
			}
		}
	}
	return false;
}

bool ZstURI::equal(const ZstURI& a, const ZstURI& b)
{
	return 	(strcmp(a.path(), b.path()) == 0);
}

bool ZstURI::operator==(const ZstURI& other) const
{
	return (strcmp(path(), other.path()) == 0);
}

bool ZstURI::operator!=(const ZstURI& other) const
{
	return !(strcmp(path(), other.path()) == 0);
}

bool ZstURI::operator<(const ZstURI& b) const
{
	bool result = std::lexicographical_compare(m_original_path.cstr, m_original_path.cstr + m_original_path.length, b.m_original_path.cstr, b.m_original_path.cstr + b.m_original_path.length);
	return result;
}

bool ZstURI::is_empty() const
{
	return m_component_count < 1;
}

pstr create_pstr(const char* p)
{
	return create_pstr(p, strlen(p));
}

pstr create_pstr(const char* p, size_t l)
{
	pstr result;
	result.length = l;
	result.cstr = (char*)malloc(result.length + 1);
	memcpy(result.cstr, p, result.length);
	result.cstr[result.length] = '\0';
	return result;
}


//--


size_t ZstURIHash::operator()(ZstURI const& k) const
{
	size_t result = 0;
	const size_t prime = 31;
	for (size_t i = 0; i < k.full_size(); ++i) {
		result = k.path()[i] + (result * prime);
	}
	return result;
}
}

std::ostream& std::operator<<(std::ostream& os, const showtime::ZstURI& uri)
{
	os << uri.path();
	return os;
}