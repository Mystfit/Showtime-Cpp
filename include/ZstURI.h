#pragma once

#include <iostream>
#include <msgpack.hpp>
#include "ZstURI.h"
#include "ZstUtils.hpp"
#include "ZstExports.h"

class ZstURI {
public:
	enum Direction {
		IN_JACK = 0,
		OUT_JACK
	};

	ZST_EXPORT ZstURI();
	ZST_EXPORT ZstURI(std::string performer, std::string instrument, std::string name, Direction direction);
	ZST_EXPORT ZstURI(const ZstURI & copy);
	ZST_EXPORT ~ZstURI();

	ZST_EXPORT std::string performer() const;
	ZST_EXPORT std::string instrument() const;
	ZST_EXPORT std::string name() const;
	ZST_EXPORT ZstURI::Direction direction() const;

	ZST_EXPORT bool operator==(const ZstURI& other);
	ZST_EXPORT bool operator!=(const ZstURI& other);
	ZST_EXPORT bool operator< (const ZstURI& b) const {
		return to_str() < b.to_str();
	}
	ZST_EXPORT std::string to_str() const;

	ZST_EXPORT static ZstURI from_str(std::string s) {
		std::vector<std::string> tokens;
		Utils::str_split(s, tokens, "/");
		return ZstURI(tokens[0], tokens[1], tokens[2], (ZstURI::Direction)std::atoi(tokens[3].c_str()));
	}
	MSGPACK_DEFINE(m_performer, m_instrument, m_name, m_direction);

private:
	std::string m_performer;
	std::string m_instrument;
	std::string m_name;
	Direction m_direction = Direction::OUT_JACK;
};

namespace msgpack {
	MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
		namespace adaptor {
			template <>
			class as<ZstURI> {
				ZstURI operator()(msgpack::object const& o) const {
					if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
					if (o.via.array.size != 4) throw msgpack::type_error();
					return ZstURI(
						o.via.array.ptr[0].as<std::string>(),
						o.via.array.ptr[1].as<std::string>(), 
						o.via.array.ptr[2].as<std::string>(), 
						o.via.array.ptr[3].as<ZstURI::Direction>()
					);
				}
			};
		} // adaptor
	} // MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
} // msgpack

MSGPACK_ADD_ENUM(ZstURI::Direction);
