#pragma once
#include <msgpack.hpp>
#include <vector>
#include <iostream>
#include <mpark/variant.hpp>
#include "ZstEnums.h"

#include "ZstExports.h"
//A ZstValue is a generic value that represents some data 
//sent from one ZstPlug to another


typedef mpark::variant<int, float, std::string> ZstValueVariant;

class ZstValue {
public:
	ZstValue();
	ZstValue(const ZstValue & other);

	ZstValue(ZstValueType t);
	~ZstValue();

	ZstValueType get_default_type();
	
	void copy(ZstValue & other);
	
	void clear();
	void append_int(int value);
	void append_float(float value);
	void append_char(const char * value);
	
	const size_t size() const;
	const int int_at(const size_t position) const;
	const float float_at(const size_t position) const;
	void char_at(char * buf, const size_t position) const;
    const size_t size_at(const size_t position) const;


protected:
	std::vector<ZstValueVariant> m_values;
	ZstValueType m_default_type;

private:
	void init();
};

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
