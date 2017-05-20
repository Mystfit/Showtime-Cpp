#include "ZstMessages.h"

ZstMessages::Kind ZstMessages::pop_message_kind_frame(zmsg_t * msg){
    return (ZstMessages::Kind)atoi(zmsg_popstr(msg));
}


//msgpack::object ZstMessages::unpack_msgpack_handle(zmsg_t * msg) {
//	msgpack::object_handle result;
//
//	zframe_t * payload = zmsg_pop(msg);
//
//	unpack(result, (char*)zframe_data(payload), zframe_size(payload));
//	return result.get();
//}

