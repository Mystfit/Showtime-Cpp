#include "ZstURI.h"
#include <exception>
#include <stdexcept>
#include <iostream>
#include "ZstUtils.hpp"

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
	int length = 0;
	int c_count = size();
	for (int i = 0; i < c_count; ++i) {
		length += components[i].length;
	}
	length += c_count-1;
	return length;
}

ZstURI ZstURI::operator+(const ZstURI & other) const
{
	int new_length = original_path.length + other.original_path.length + 1;

	char * new_path = (char*)calloc(new_length+1, sizeof(char));
	strncpy(new_path, original_path.cstr, original_path.length);

	strncat(new_path, "/", 1);
	strncat(new_path, other.original_path.cstr, other.original_path.length);
	
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
	if (start > size() || end >= size()) {
		throw std::range_error("Start or end exceeds path length");
	}

	int new_length = 0;

	for (int i = start; i <= end; ++i) {
		new_length += components[i].length;
	}
	new_length += end - start;
	char * new_path = (char*)calloc(new_length + 1, sizeof(char));
	strncpy(new_path, components[start].cstr, components[start].length);
	
	for (int i = start + 1; i <= end; ++i) {
		strncat(new_path, "/", 1);
		strncat(new_path, components[i].cstr, components[i].length);
	}

	ZstURI result = ZstURI(new_path);
	free(new_path);
	return result;
}

bool ZstURI::contains(const ZstURI & compare)
{
	int shortest = (compare.size() > this->size()) ? this->size() : compare.size();
	int largest = (compare.size() < this->size()) ? this->size() : compare.size();
	int contiguous = 0;

	for (int i = 0; i < largest; ++i) {
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

	if (strcmp(original_path.cstr, "") == 0)
		return;

	int c_count = 0;
	int offset = 0;

	for (int i = 0; i < segmented_path.length; i++) {
		if (segmented_path.cstr[i] == DELIM) {
			segmented_path.cstr[i] = '\0';
			c_count++;
		}
	}
	
	pstr comp;
	comp.length = strlen(segmented_path.cstr);
	comp.cstr = segmented_path.cstr + offset;
	components[component_count] = comp;
	offset += comp.length + 1;
	component_count++;

	while (c_count > 0) {
		comp.length = strlen(segmented_path.cstr + offset);
		comp.cstr = segmented_path.cstr + offset;
		components[component_count] = comp;
		component_count++;
		offset += comp.length + 1;
		c_count--;
	}
}

pstr ZstURI::create_pstr(const char * p)
{
	pstr result;
	result.length = strlen(p);
	result.cstr = (char*)calloc(result.length+1, sizeof(char));

	strncpy(result.cstr, p, result.length+1);
	return result;
}

size_t std::hash<ZstURI>::operator()(const ZstURI & k) const
{
	return Utils::hash_c_string(k.path(), k.full_size());
}
