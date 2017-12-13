#pragma once
#include <memory>
#include "ZstStreamable.h"
#include "ZstExports.h"

struct pstr {
	unsigned length;
	char *cstr;
};

#define PSTR(x) ((struct pstr){(unsigned)sizeof(x) - 1, x})
#define MAX_PATH_LEN 255
#define DELIM '/'

class ZstURI : public ZstStreamable
{

public:
	ZST_EXPORT ZstURI();
	ZST_EXPORT ~ZstURI();
	ZST_EXPORT ZstURI(const ZstURI & copy);
	ZST_EXPORT ZstURI(const char * path);
	ZST_EXPORT ZstURI(const char * path, int len);
	ZST_EXPORT const char * path() const;
	ZST_EXPORT char * segment(size_t index);
	ZST_EXPORT const int size() const;
	ZST_EXPORT const int full_size() const;

	ZST_EXPORT ZstURI operator+(const ZstURI & other) const;
	ZST_EXPORT ZstURI & operator=(const ZstURI & other);
	ZST_EXPORT ZstURI range(int start, int end) const;
	ZST_EXPORT bool contains(const ZstURI & compare) const;
	ZST_EXPORT static bool equal(const ZstURI & a, const ZstURI & b);
	ZST_EXPORT bool operator==(const ZstURI & other) const;
	ZST_EXPORT bool operator!=(const ZstURI & other) const;
	ZST_EXPORT bool operator< (const ZstURI& b) const;
	ZST_EXPORT bool is_empty();

	ZST_EXPORT virtual void write(std::stringstream & buffer) override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;

protected:
	void split(void);

	pstr create_pstr(const char * p);
	pstr create_pstr(const char * p, int l);

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