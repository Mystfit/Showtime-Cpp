#pragma once

#include <iostream>
#include <string>
#include <msgpack.hpp>
#include "ZstUtils.hpp"
#include "ZstExports.h"

class ZstURI {
public:
	enum Direction {
		IN_JACK = 0,
		OUT_JACK
	};

	ZST_EXPORT ZstURI();
	ZST_EXPORT ZstURI(const std::string performer, const std::string  instrument, const std::string  name, Direction direction);
	ZST_EXPORT ~ZstURI();
	ZST_EXPORT static ZstURI * create(const char *  performer, const char *  instrument, const char *  name, Direction direction);
	ZST_EXPORT static void destroy(ZstURI * uri);

	ZST_EXPORT std::string performer();
	ZST_EXPORT std::string instrument();
	ZST_EXPORT std::string name();

	ZST_EXPORT const char * performer_char();
	ZST_EXPORT const char * instrument_char();
	ZST_EXPORT const char * name_char();
	ZST_EXPORT const ZstURI::Direction direction();

	ZST_EXPORT bool operator==(const ZstURI& other);
	ZST_EXPORT bool operator!=(const ZstURI& other);
	ZST_EXPORT bool operator< (const ZstURI& b) const;

	ZST_EXPORT const std::string to_str() const;
	ZST_EXPORT const char * to_char() const;
	ZST_EXPORT static ZstURI from_str(const char * s);
	MSGPACK_DEFINE(m_performer, m_instrument, m_name, m_direction);

private:
	std::string m_performer;
	std::string m_instrument;
	std::string m_name;
	Direction m_direction;
};

MSGPACK_ADD_ENUM(ZstURI::Direction);
