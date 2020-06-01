#pragma once

#include "ZstExports.h"


namespace showtime {

    class ZST_CLASS_EXPORTED ZstEventModule {
        ZST_EXPORT virtual void init_adaptors() = 0;
        ZST_EXPORT virtual void process_events() = 0;
        ZST_EXPORT virtual void flush_events() = 0;
    };
}