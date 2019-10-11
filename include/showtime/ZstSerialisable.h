#pragma once

#include "schemas/graph_types_generated.h"

#include "ZstExports.h"

namespace showtime {
    
    template<typename Buffer_t, typename BufferBuilder_t>
    class ZST_CLASS_EXPORTED ZstSerialisable {
    public:
        ZST_EXPORT virtual flatbuffers::Offset<Buffer_t> serialize(BufferBuilder_t & buffer_builder) const = 0;
        ZST_EXPORT virtual void deserialize(const Buffer_t* buffer) = 0;
    };
}
