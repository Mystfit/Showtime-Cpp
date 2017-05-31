#include "ZstMessages.h"

ZstMessages::Kind ZstMessages::pop_message_kind_frame(zmsg_t * msg){
    return (ZstMessages::Kind)atoi(zmsg_popstr(msg));
}
