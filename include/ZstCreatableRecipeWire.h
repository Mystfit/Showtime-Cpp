//
//  ZstCreatableWire.h
//  Showtime
//
//  Created by Byron Mallett on 4/10/17.
//
//

#pragma once

#include <msgpack.hpp>
#include "ZstCreatable.h"

class ZstCreatableRecipeWire : public ZstCreatableRecipe {
public:
    ZstCreatableRecipeWire();
    ZstCreatableRecipeWire(const ZstCreatableRecipeWire &copy);
    ZstCreatableRecipeWire(const ZstCreatableRecipe &copy);
    ~ZstCreatableRecipeWire();
    
    template <typename Packer>
    void msgpack_pack(Packer& pk) const {
        pk.pack_array(3);
        
        //Performer
        pk.pack_str(performer.size());
        pk.pack_str_body(performer);
        
        //Entity type
        pk.pack_str(entity_type.size());
        pk.pack_str_body(entity_type);
        
        //Valid parents
        pk.pack_array(valid_parent_types.size());
        for(auto s : valid_parent_types){
            pk.pack_str(s.size());
            pk.pack_str_body(s);
        }
    }
    
    void msgpack_unpack(msgpack::object o);
};
