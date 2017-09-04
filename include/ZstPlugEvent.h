#pragma once

#include "ZstEvent.h"
#include "ZstValue.h"

class ZstPlugEvent : public ZstEvent {
public:
	ZstPlugEvent(ZstURI uri, ZstValue & value);
	ZST_EXPORT ~ZstPlugEvent();
	ZST_EXPORT ZstValue * value();
private:
	ZstValue * m_value;
};