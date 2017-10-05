//
//  ZstCreatableWire.cpp
//  Showtime
//
//  Created by Byron Mallett on 4/10/17.
//
//

#include <stdio.h>
#include "ZstCreatableRecipeWire.h"

ZstCreatableRecipeWire::ZstCreatableRecipeWire() : ZstCreatableRecipe()
{
}

ZstCreatableRecipeWire::ZstCreatableRecipeWire(const ZstCreatableRecipeWire & copy) : ZstCreatableRecipe(copy)
{
}

ZstCreatableRecipeWire::ZstCreatableRecipeWire(const ZstCreatableRecipe & copy) : ZstCreatableRecipe(copy)
{
}

void ZstCreatableRecipeWire::msgpack_unpack(msgpack::object o)
{
    // check if received structure is an array
    if (o.type != msgpack::type::ARRAY) { throw msgpack::type_error(); }
    
    const size_t size = o.via.array.size;
    
    // sanity check
    if (size < 3) return;
    
    performer = o.via.array.ptr[0].as<std::string>();
    entity_type = o.via.array.ptr[1].as<std::string>();
    valid_parent_types = o.via.array.ptr[2].as<std::vector<std::string> >();
}
