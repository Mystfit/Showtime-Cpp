#pragma once

#include <iostream>
#include <string>
#include "ZstUtils.hpp"
#include "ZstExports.h"

#define MAX_PATH_LEN 20

class ZstURI {
public:
	ZST_EXPORT ZstURI();
	ZST_EXPORT ZstURI(const ZstURI &copy);
	ZST_EXPORT ZstURI(const char *  path);
	ZST_EXPORT ~ZstURI();

	ZST_EXPORT const char * path();
	ZST_EXPORT const size_t size() const;
	ZST_EXPORT ZstURI range(int start, int end) const;

	ZST_EXPORT bool contains(ZstURI compare);
	ZST_EXPORT const char * segment(int index) const;
	ZST_EXPORT bool operator==(const ZstURI& other);
	ZST_EXPORT bool operator!=(const ZstURI& other);
	ZST_EXPORT bool operator< (const ZstURI& b) const;
	ZST_EXPORT bool is_empty();

	ZST_EXPORT static ZstURI join(ZstURI a, ZstURI b);
	ZST_EXPORT static ZstURI from_char(const char * s);


protected:
	Str255 m_path;
	Str255 m_combined_path;
	long m_path_offsets[MAX_PATH_LEN];
	int m_num_path_components;
	void build_split_path(const char * path);
};
