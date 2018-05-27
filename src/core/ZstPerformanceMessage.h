#pragma once

#include "ZstMessage.h"
#include "liasons/ZstPlugLiason.hpp"

class ZstPerformanceMessage : public ZstMessage, public ZstPlugLiason {
public:    
    ZST_EXPORT ZstPerformanceMessage * init_performance_message(const ZstURI & sender);
    ZST_EXPORT ZstPerformanceMessage * init_performance_message(ZstOutputPlug * plug);
    ZST_EXPORT void unpack(zmsg_t * msg) override;
    ZST_EXPORT const ZstURI & sender();

private:
    ZstURI m_sender;
};
