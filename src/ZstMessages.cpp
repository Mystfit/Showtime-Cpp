#include "ZstMessages.h"

 ZstMessages::Kind ZstMessages::pop_message_kind_frame(zmsg_t * msg){
     char * kind_str = zmsg_popstr(msg);
     ZstMessages::Kind k = static_cast<ZstMessages::Kind>(kind_str[0]);
     zstr_free(&kind_str);
     return k;
 }
