#pragma once

//A ZstValue is a generic value that represents some data 
//sent from one ZstPlug to another

#include <vector>
#include <iostream>
#include <ZstConstants.h>
#include <ZstSerialisable.h>
#include <ZstExports.h>
#include <mutex>

#include <boost/bimap/bimap.hpp>
#include <boost/bimap/list_of.hpp>
#include <boost/assign/list_of.hpp>


//Forward declare variant
namespace mpark {
	template <typename... Ts>
	class variant;
}
typedef mpark::variant<int, float, std::string> ZstValueVariant;


namespace ZstValueDetails {
	class ZstValueIntVisitor
	{
	public:
		int operator()(int i) const;
		int operator()(float f) const;
		int operator()(const std::string & str) const;
	};

	class ZstValueFloatVisitor
	{
	public:
		float operator()(int i) const;
		float operator()(float f) const;
		float operator()(const std::string & str) const;
	};

	class ZstValueStrVisitor
	{
	public:
		std::string operator()(int i) const;
		std::string operator()(float f) const;
		std::string operator()(const std::string & str) const;
	};

	enum ZstValueFields {
		VALUES,
		DEFAULT_TYPE
	};

	typedef boost::bimaps::bimap<ZstValueFields, std::string> ZstValueFieldsMap;
	static ZstValueFieldsMap ZstValueFieldNames = boost::assign::list_of<ZstValueFieldsMap::relation>
		(DEFAULT_TYPE, "d")
		(VALUES, "v");

	static const std::string & get_value_field_name(const ZstValueFields & field) {
		return ZstValueFieldNames.left.at(field);
	}

	static const ZstValueFields & get_value_field(const std::string field_str) {
		return ZstValueFieldNames.right.at(field_str);
	}
}


class ZstValue : public ZstSerialisable {
public:
	ZST_EXPORT ZstValue();
	ZST_EXPORT ZstValue(const ZstValue & other);

	ZST_EXPORT ZstValue(ZstValueType t);
	ZST_EXPORT virtual ~ZstValue();

	ZST_EXPORT ZstValueType get_default_type() const;
	
	ZST_EXPORT void copy(const ZstValue & other);
	
	ZST_EXPORT void clear();
	ZST_EXPORT void append_int(int value);
	ZST_EXPORT void append_float(float value);
	ZST_EXPORT void append_char(const char * value);
	
	ZST_EXPORT const size_t size() const;
	ZST_EXPORT const int int_at(const size_t position) const;
	ZST_EXPORT const float float_at(const size_t position) const;
	ZST_EXPORT void char_at(char * buf, const size_t position) const;
	ZST_EXPORT const size_t size_at(const size_t position) const;

	//Serialisation
	ZST_EXPORT void write(std::stringstream & buffer) const override;
	ZST_EXPORT void read(const char * buffer, size_t length, size_t & offset) override;
	ZST_EXPORT void write_json(json & buffer) const override;
	ZST_EXPORT void read_json(const json & buffer) override;

protected:
	std::vector<ZstValueVariant> m_values;
	ZstValueType m_default_type;

private:
	std::mutex m_lock;
};


