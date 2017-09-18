#include "Uri.h"

Uri::Uri() {}
Uri::~Uri() {
	free(original_path.cstr);
	free(segmented_path.cstr);
}

Uri::Uri(Uri & copy)
{
	original_path = create_pstr(copy.original_path.cstr);
	segmented_path = create_pstr(copy.original_path.cstr);
	split();
}

//

Uri::Uri(char *path)
{
	original_path = create_pstr(path);
	segmented_path = create_pstr(path);
	split();
}

//
char* Uri::path()
{
	return original_path.cstr;
}

char * Uri::segment(size_t index)
{
	return components[index].cstr;
}

int Uri::size()
{
	return component_count;
}

Uri Uri::operator+(const Uri & other) const
{
	int new_length = original_path.length + other.original_path.length + 1;

	char * new_path = (char*)calloc(new_length+1, sizeof(char));
	strncpy(new_path, original_path.cstr, original_path.length);

	strncat(new_path, "/", 1);
	strncat(new_path, other.original_path.cstr, other.original_path.length);
	
	Uri result = Uri(new_path);
	free(new_path);
	new_path = 0;
	return result;
}

void Uri::split()
{
	component_count = 0;
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

	components[component_count++] = comp;
	offset += comp.length + 1;

	while (c_count > 0) {
		comp.length = strlen(segmented_path.cstr + offset);
		comp.cstr = segmented_path.cstr + offset;
		components[component_count++] = comp;
		offset += comp.length + 1;
		c_count--;
	}
}

pstr Uri::create_pstr(char * p)
{
	pstr result;
	result.length = strlen(p);
	result.cstr = (char*)calloc(result.length+1, sizeof(char));

	strncpy(result.cstr, p, result.length+1);
	return result;
}
