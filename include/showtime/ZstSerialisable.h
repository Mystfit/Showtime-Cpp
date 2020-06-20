#pragma once

#include <flatbuffers/flatbuffers.h>
#include <showtime/schemas/messaging/graph_types_generated.h>
#include <showtime/ZstExports.h>

namespace showtime {
 
    template<typename Buffer_t, typename BufferPart_t>
    class ZST_CLASS_EXPORTED ZstSerialisable {
    public:
        ZST_EXPORT virtual void serialize_partial(flatbuffers::Offset<BufferPart_t> & destination_offset, flatbuffers::FlatBufferBuilder & buffer_builder) const = 0;
        ZST_EXPORT virtual flatbuffers::uoffset_t serialize(flatbuffers::FlatBufferBuilder & buffer_builder) const = 0;
        
        ZST_EXPORT virtual void deserialize_partial(const BufferPart_t* buffer) = 0;
        ZST_EXPORT virtual void deserialize(const Buffer_t* buffer) = 0;
    };
}
