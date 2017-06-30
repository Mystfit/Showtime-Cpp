#pragma once
#include "ZstExports.h"
#include "ZstURI.h"
#include "ZstEvent.h"

class ZstCable {
public:
	ZST_EXPORT ZstCable();
	ZST_EXPORT ZstCable(const ZstCable & copy);
	ZST_EXPORT ZstCable(const ZstEvent & e);
	ZST_EXPORT ZstCable(const ZstURI input, const ZstURI );
	ZST_EXPORT ~ZstCable();

	ZST_EXPORT bool operator==(const ZstURI& other);
	ZST_EXPORT bool operator==(const ZstCable& other);
	ZST_EXPORT bool operator!=(const ZstURI& other);

	ZST_EXPORT ZstURI & get_input();
	ZST_EXPORT ZstURI & get_output();
private:
	ZstURI m_input;
	ZstURI m_output;
};
