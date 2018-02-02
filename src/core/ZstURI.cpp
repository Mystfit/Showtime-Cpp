#include <exception>
#include <stdexcept>
#include <iostream>
#include <msgpack.hpp>
#include <ZstURI.h>

ZstURI::ZstURI() : m_component_count(0) {
	m_original_path = create_pstr("");
	m_segmented_path = create_pstr("");
	m_component_count = 0;
}


ZstURI::~ZstURI() {
	free(m_original_path.cstr);
	free(m_segmented_path.cstr);
}

ZstURI::ZstURI(const ZstURI & copy) : 
	m_component_count(0)
{
	m_original_path = create_pstr(copy.m_original_path.cstr, copy.m_original_path.length);
	m_segmented_path = create_pstr(copy.m_original_path.cstr, copy.m_original_path.length);
    init();
}

ZstURI & ZstURI::operator=(const ZstURI & other)
{
	m_original_path = create_pstr(other.m_original_path.cstr, other.m_original_path.length);
	m_segmented_path = create_pstr(other.m_original_path.cstr, other.m_original_path.length);
	init();
	return *this;
}

//

ZstURI::ZstURI(const char * path)
{
	m_original_path = create_pstr(path);
	m_segmented_path = create_pstr(path);
	init();
}

ZstURI::ZstURI(const char * path, size_t len)
{
	m_original_path = create_pstr(path, len);
	m_segmented_path = create_pstr(path, len);
	init();
}

void ZstURI::init()
{
	m_component_count = 0;

	if (m_original_path.cstr[0] == 0)
		return;

	pstr comp;

	comp.cstr = m_segmented_path.cstr;
	size_t len = 0;
	for (size_t i = 0; i < m_segmented_path.length; i++) {
		if (m_segmented_path.cstr[i] == DELIM) {
			m_segmented_path.cstr[i] = '\0';
			comp.length = len;
			m_components[m_component_count++] = comp;
			comp.cstr = &m_segmented_path.cstr[i + 1];
			len = 0;
		}
		else {
			len++;
		}

	}

	//If we only have one component, don't need to truncate length
	comp.length = len;
	m_components[m_component_count++] = comp;
}

//
const char * ZstURI::path() const
{
	return m_original_path.cstr;
}

char * ZstURI::segment(size_t index)
{
	if(index >= m_component_count) {
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

ZstURI ZstURI::operator+(const ZstURI & other) const
{
	size_t new_length = m_original_path.length + other.m_original_path.length + 2;

	char * new_path = (char*)malloc(new_length+1);
    memcpy(new_path, m_original_path.cstr, m_original_path.length);
	new_path[m_original_path.length] = '\0';

	strncat(new_path, "/", 1);
	strncat(new_path, other.m_original_path.cstr, other.m_original_path.length);
	new_path[new_length-1] = '\0';

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
	length += end - start;

	char * start_s = &m_original_path.cstr[start_position];
	ZstURI result = ZstURI(start_s, length);
	return result;
}

ZstURI ZstURI::parent() const
{
	int s = static_cast<int>(this->size());
	int parent_index = s - 2;
	if (parent_index < 0) {
		throw std::out_of_range("URI has no parent");
	}
	return range(0, parent_index);
}

ZstURI ZstURI::first() const
{
	if (size() < 1)
		throw std::out_of_range("URI is empty");
	return ZstURI(m_components[0].cstr);
}

ZstURI ZstURI::last()
{
	return ZstURI(m_components[size()-1].cstr);
}

bool ZstURI::contains(const ZstURI & compare) const
{
	if (compare.size() > size()) {
		return false;
	}

	int s = static_cast<int>(size());
	int other_s = static_cast<int>(compare.size());
	int shortest = std::min(other_s,s);
	int largest = std::max(other_s, s);
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

bool ZstURI::equal(const ZstURI & a, const ZstURI & b)
{
	return 	(strcmp(a.path(), b.path()) == 0);
}

bool ZstURI::operator==(const ZstURI & other) const
{
	return (strcmp(path(), other.path()) == 0);
}

bool ZstURI::operator!=(const ZstURI & other) const
{
	return !(strcmp(path(), other.path()) == 0);
}

bool ZstURI::operator<(const ZstURI & b) const
{
	return strcmp(b.m_original_path.cstr, m_original_path.cstr) < 0 ? false: true;
}

bool ZstURI::is_empty()
{
	return m_component_count < 1;
}

ZstURI::pstr ZstURI::create_pstr(const char * p)
{
	return create_pstr(p, strlen(p));
}

ZstURI::pstr ZstURI::create_pstr(const char * p, size_t l)
{
	pstr result;
	result.length = l + 1;
	result.cstr = (char*)malloc(result.length);
	memcpy(result.cstr, p, result.length-1);
	result.cstr[result.length-1] = '\0';

	return result;
}


//--


size_t ZstURIHash::operator()(ZstURI const & k) const
{
	size_t result = 0;
	const size_t prime = 31;
	for (size_t i = 0; i < k.full_size(); ++i) {
		result = k.path()[i] + (result * prime);
	}
	return result;
}
