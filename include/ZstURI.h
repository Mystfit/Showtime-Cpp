#pragma once

#include <iostream>
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
	ZST_EXPORT ZstURI(const char * performer, const char * instrument, const char * name, const Direction direction);
	ZST_EXPORT ZstURI(const ZstURI & copy);
	ZST_EXPORT ~ZstURI();

	ZST_EXPORT const char * performer();
	ZST_EXPORT const char * instrument();
	ZST_EXPORT const char * name();
	ZST_EXPORT const ZstURI::Direction direction();

	ZST_EXPORT bool operator==(const ZstURI& other);
	ZST_EXPORT bool operator!=(const ZstURI& other);
	ZST_EXPORT bool operator< (const ZstURI& b) const {
		return std::string(to_char()) < std::string(b.to_char());
	}
	ZST_EXPORT const char * to_char() const;

	ZST_EXPORT static ZstURI from_char(const char * s) {
		std::vector<std::string> tokens;
		Utils::str_split(std::string(s), tokens, "/");
		return ZstURI(tokens[0].c_str(), tokens[1].c_str(), tokens[2].c_str(), (ZstURI::Direction)std::atoi(tokens[3].c_str()));
	}
	MSGPACK_DEFINE(m_performer, m_instrument, m_name, m_direction);

private:
	std::string m_performer;
	std::string m_instrument;
	std::string m_name;
	Direction m_direction;
};

MSGPACK_ADD_ENUM(ZstURI::Direction);
