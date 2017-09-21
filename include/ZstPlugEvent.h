#pragma once

#include "ZstEvent.h"
#include "ZstValue.h"

class ZstPlugEvent : public ZstEvent {
public:
	ZstPlugEvent(const ZstURI & uri, const ZstValue & value);
	~ZstPlugEvent();
	ZstValue * value();
private:
	ZstValue * m_value;
};