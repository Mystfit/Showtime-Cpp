#pragma once

#include <iostream>
#include <string>
#include "ZstUtils.hpp"
#include "ZstExports.h"

class ZstURI {
public:
	enum Direction {
		NONE = 0,
		IN_JACK,
		OUT_JACK
	};

	ZST_EXPORT ZstURI();
	ZST_EXPORT ZstURI(const ZstURI &copy);
	ZST_EXPORT ZstURI(const char *  performer, const char *  instrument, const char *  name, Direction direction);
	ZST_EXPORT ~ZstURI();

	ZST_EXPORT static ZstURI * create(const char *  performer, const char *  instrument, const char *  name, Direction direction);
	ZST_EXPORT static ZstURI * create_empty();

	ZST_EXPORT static void destroy(ZstURI * uri);

	ZST_EXPORT const std::string performer() const;
	ZST_EXPORT const std::string instrument() const;
	ZST_EXPORT const std::string name() const;

	ZST_EXPORT const char * performer_char();
	ZST_EXPORT const char * instrument_char();
	ZST_EXPORT const char * name_char();
	ZST_EXPORT const ZstURI::Direction direction();

	ZST_EXPORT bool operator==(const ZstURI& other);
	ZST_EXPORT bool operator!=(const ZstURI& other);
	ZST_EXPORT bool operator< (const ZstURI& b) const;
	ZST_EXPORT bool is_empty();

	ZST_EXPORT const char * to_char() const;
	ZST_EXPORT static ZstURI from_char(const char * s);

protected:
	char m_performer[255];
	char m_instrument[255];
	char m_name[255];
	Direction m_direction;
	char m_combined_char[255];
	bool m_created_combined_char = false;
    void build_combined_char();
};
