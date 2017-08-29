#include "ZstMessages.h"

 ZstMessages::Kind ZstMessages::pop_message_kind_frame(zmsg_t * msg){
     char * kind_str = zmsg_popstr(msg);
     ZstMessages::Kind k = (ZstMessages::Kind)std::atoi(kind_str);
     zstr_free(&kind_str);
     return k;
 }
