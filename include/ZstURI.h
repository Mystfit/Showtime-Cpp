#pragma once
#include <memory>
#include <ZstExports.h>

struct pstr {
	size_t length;
	char *cstr;
};

#define PSTR(x) ((struct pstr){(unsigned)sizeof(x) - 1, x})
#define MAX_PATH_LEN 255
#define DELIM '/'

class ZstURI
{
public:
	ZST_EXPORT ZstURI();
	ZST_EXPORT ~ZstURI();
	ZST_EXPORT ZstURI(const ZstURI & copy);
	ZST_EXPORT ZstURI(const char * path);
	ZST_EXPORT ZstURI(const char * path, size_t len);
	ZST_EXPORT const char * path() const;
	ZST_EXPORT char * segment(size_t index);
	ZST_EXPORT const size_t size() const;
	ZST_EXPORT const size_t full_size() const;

	ZST_EXPORT ZstURI operator+(const ZstURI & other) const;
	ZST_EXPORT ZstURI & operator=(const ZstURI & other);
	ZST_EXPORT ZstURI range(size_t start, size_t end) const;

	//Returns a URI containing the parent
	ZST_EXPORT ZstURI parent() const;

	//Returns a URI containing the first segment
	ZST_EXPORT ZstURI first() const;

	//Return a URI containing the last segment
	ZST_EXPORT ZstURI last();

	ZST_EXPORT bool contains(const ZstURI & compare) const;
	ZST_EXPORT static bool equal(const ZstURI & a, const ZstURI & b);
	ZST_EXPORT bool operator==(const ZstURI & other) const;
	ZST_EXPORT bool operator!=(const ZstURI & other) const;
	ZST_EXPORT bool operator< (const ZstURI& b) const;
	ZST_EXPORT bool is_empty();

protected:
	void split(void);

	pstr create_pstr(const char * p);
	pstr create_pstr(const char * p, size_t l);

	pstr original_path;
	pstr segmented_path;

	int  component_count;
	pstr components[MAX_PATH_LEN];

};

namespace std
{
	template <>
	struct hash<ZstURI>
	{
		ZST_EXPORT size_t operator()(const ZstURI& k) const;
	};
}