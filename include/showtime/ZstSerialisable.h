#pragma once

#include <flatbuffers/flatbuffers.h>
#include "schemas/graph_types_generated.h"

#include "ZstExports.h"

namespace showtime {
    
    typedef uint8_t BufferPtr;
    
    template<typename T>
    class ZST_CLASS_EXPORTED ZstSerialisable {
    public:
        ZST_EXPORT virtual void serialize(flatbuffers::Offset<T> & serialized_offset, flatbuffers::FlatBufferBuilder & buffer_builder) const = 0;
        ZST_EXPORT virtual void deserialize(const T* buffer) = 0;
    };
}
