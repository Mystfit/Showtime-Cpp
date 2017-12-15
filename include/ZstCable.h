#pragma once
#include "ZstExports.h"
#include "ZstURI.h"
#include "ZstStreamable.h"
#include "entities/ZstPlug.h"

class ZstCable : public ZstStreamable {
public:
	ZST_EXPORT ZstCable();
	ZST_EXPORT ZstCable(const ZstCable & copy);
	ZST_EXPORT ZstCable(ZstPlug * input_plug, ZstPlug * output_plug);
	ZST_EXPORT ~ZstCable();

	ZST_EXPORT bool operator==(const ZstCable & other);
	ZST_EXPORT bool operator!=(const ZstCable & other);
	ZST_EXPORT bool is_attached(const ZstURI & uri);
	ZST_EXPORT bool is_attached(const ZstURI & uriA, const ZstURI & uriB);

	ZST_EXPORT ZstURI & get_input();
	ZST_EXPORT ZstURI & get_output();

	ZST_EXPORT virtual void write(std::stringstream & buffer) override;
	ZST_EXPORT virtual void read(const char * buffer, size_t length, size_t & offset) override;
protected:
	ZstURI m_input;
	ZstURI m_output;
};
