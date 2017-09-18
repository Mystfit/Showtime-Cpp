#pragma once
#include <memory>
#include "ZstExports.h"

struct pstr {
	unsigned length;
	char *cstr;
};

#define PSTR(x) ((struct pstr){(unsigned)sizeof(x) - 1, x})

struct pstr pstr_append(struct pstr out,
	const struct pstr a,
	const struct pstr b)
{
	memcpy(out.cstr, a.cstr, a.length);
	memcpy(out.cstr + a.length, b.cstr, b.length + 1);
	out.length = a.length + b.length;
	return out;
}

#define PSTR_APPEND(a,b) pstr_append((struct pstr){0, alloca(a.length + b.length + 1)}, a, b)
#define MAX_PATH_LEN 255
#define DELIM '/'

class Uri
{

public:
	ZST_EXPORT Uri();
	ZST_EXPORT ~Uri();
	ZST_EXPORT Uri(Uri &copy);
	ZST_EXPORT Uri(char *p);
	ZST_EXPORT char *path();
	ZST_EXPORT char * segment(size_t index);
	ZST_EXPORT int size();
	ZST_EXPORT Uri operator+(const Uri & other) const;

protected:
	void split(void);

	pstr create_pstr(char* p);

	pstr original_path;
	pstr segmented_path;

	int  component_count;
	pstr components[MAX_PATH_LEN];

};