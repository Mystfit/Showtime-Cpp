#include <exception>
#include <stdexcept>
#include <iostream>
#include "ZstURI.h"
#include "ZstUtils.hpp"
#include "msgpack.hpp"

ZstURI::ZstURI() : component_count(0) {
	original_path = create_pstr("");
	segmented_path = create_pstr("");
	component_count = 0;
}


ZstURI::~ZstURI() {
	free(original_path.cstr);
	free(segmented_path.cstr);
}

ZstURI::ZstURI(const ZstURI & copy) : component_count(0)
{
	original_path = create_pstr(copy.original_path.cstr);
	segmented_path = create_pstr(copy.original_path.cstr);
    split();
}

//

ZstURI::ZstURI(const char * path)
{
	original_path = create_pstr(path);
	segmented_path = create_pstr(path);
	split();
}

ZstURI::ZstURI(const char * path, int len)
{
	original_path = create_pstr(path, len);
	segmented_path = create_pstr(path, len);
	split();
}

//
const char * ZstURI::path() const
{
	return original_path.cstr;
}

char * ZstURI::segment(size_t index)
{
	if(index >= component_count) {
		throw std::range_error("Start or end index out of range of path components");
	}
	return components[index].cstr;
}

const int ZstURI::size() const
{
	return component_count;
}

const int ZstURI::full_size() const
{
	return strlen(original_path.cstr);
}

ZstURI ZstURI::operator+(const ZstURI & other) const
{
	int new_length = original_path.length + other.original_path.length + 1;

	char * new_path = (char*)malloc(new_length+1);
	strncpy(new_path, original_path.cstr, original_path.length);
	new_path[original_path.length] = '\0';

	strncat(new_path, "/", 1);
	strncat(new_path, other.original_path.cstr, other.original_path.length);
	new_path[new_length] = '\0';

	ZstURI result = ZstURI(new_path);
	free(new_path);
	new_path = 0;
	return result;
}

ZstURI & ZstURI::operator=(const ZstURI & other)
{
	original_path = create_pstr(other.original_path.cstr);
	segmented_path = create_pstr(other.original_path.cstr);
    split();
	return *this;
}

ZstURI ZstURI::range(int start, int end) const
{
	if ((end - start) > size())
		throw std::range_error("Start or end exceeds path length");

	int index = 0;
	int start_position = 0;

	for (index; index < start; index++)
		start_position += components[index].length + 1;

	int length = 0;
	for (index = start; index <= end; index++)
		length += components[index].length;
	length += end - start;

	char * start_s = &original_path.cstr[start_position];
	ZstURI result = ZstURI(start_s, length);
	return result;
}

ZstURI ZstURI::parent() const
{
	if (size() - 2 < 0)
		throw std::out_of_range("URI has no parent");
	return range(0, size() - 2);
}

ZstURI ZstURI::first() const
{
	if (size() < 1)
		throw std::out_of_range("URI is empty");
	return ZstURI(components[0].cstr);
}

ZstURI ZstURI::last()
{
	return ZstURI(components[size()-1].cstr);
}

bool ZstURI::contains(const ZstURI & compare) const
{
	if (compare.size() > size()) {
		return false;
	}

	int shortest = std::min(compare.size(), size());
	int largest = std::max(compare.size(), size());
	int contiguous = 0;

	for (int i = 0; i < shortest; ++i) {
		if (i < compare.size()) {
			if (strcmp(compare.components[i].cstr, this->components[i].cstr) == 0) {
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
	return strcmp(b.original_path.cstr, original_path.cstr) < 0 ? false: true;
}

bool ZstURI::is_empty()
{
	return component_count < 1;
}

void ZstURI::split()
{
	component_count = 0;

	if (original_path.cstr[0] == 0)
		return;

	pstr comp;

	comp.cstr = segmented_path.cstr;
	int i = 0;
	int len = 0;
	for (i; i < segmented_path.length; i++) {
		if (segmented_path.cstr[i] == DELIM) {
			segmented_path.cstr[i] = '\0';
			comp.length = len;
			components[component_count++] = comp;
			comp.cstr = &segmented_path.cstr[i + 1];
			len = 0;
		}
		else {
			len++;
		}
		
	}
	//If we only have one component, don't need to truncate length
	comp.length = len;
	components[component_count++] = comp;
}

pstr ZstURI::create_pstr(const char * p)
{
	return create_pstr(p, strlen(p));
}

pstr ZstURI::create_pstr(const char * p, int l)
{
	pstr result;
	result.length = l;
	result.cstr = (char*)malloc(result.length + 1);
	strncpy(result.cstr, p, result.length + 1);
	result.cstr[result.length] = '\0';

	return result;
}

size_t std::hash<ZstURI>::operator()(const ZstURI & k) const
{
	return Utils::hash_c_string(k.path(), k.full_size());
}
