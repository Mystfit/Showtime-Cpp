#pragma once
#include <msgpack.hpp>
#include <vector>
#include <iostream>
#include <boost/variant.hpp>
#include "Showtime.h"
#include "ZstExports.h"
//A ZstValue is a generic value that represents some data 
//sent from one ZstPlug to another

typedef boost::variant<int, float, std::string> ZstValueVariant;

class ZstValue {
public:
	ZST_EXPORT ZstValue();
	ZST_EXPORT ZstValue(const ZstValue & other);

	ZST_EXPORT ZstValue(ZstValueType t);
	ZST_EXPORT ~ZstValue();

	ZST_EXPORT ZstValueType get_default_type();
	
	ZST_EXPORT void clear();
	ZST_EXPORT void append_int(int value);
	ZST_EXPORT void append_float(float value);
	ZST_EXPORT void append_char(const char * value);
	
	ZST_EXPORT const size_t size() const;
	ZST_EXPORT const int int_at(const size_t position) const;
	ZST_EXPORT const float float_at(const size_t position) const;
	ZST_EXPORT void char_at(char * buf, const size_t position) const;
    ZST_EXPORT const size_t size_at(const size_t position) const;


protected:
	std::vector<ZstValueVariant> m_values;
	ZstValueType m_default_type;

private:
	void init();
};

class ZstValueIntVisitor : public boost::static_visitor<int>
{
public:
	int operator()(int i) const;
	int operator()(float f) const;
	int operator()(const std::string & str) const;
};

class ZstValueFloatVisitor : public boost::static_visitor<float>
{
public:
	float operator()(int i) const;
	float operator()(float f) const;
	float operator()(const std::string & str) const;
};

class ZstValueStrVisitor : public boost::static_visitor<std::string>
{
public:
	std::string operator()(int i) const;
	std::string operator()(float f) const;
	std::string operator()(const std::string & str) const;
};
