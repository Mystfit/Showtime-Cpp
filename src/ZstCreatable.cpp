//
//  ZstCreatable.cpp
//  Showtime
//
//  Created by Byron Mallett on 3/10/17.
//
//

#include <ZstCreatable.h>

ZstCreateableKitchen::ZstCreateableKitchen(const char * entity_type) : m_entity_type(""){
    m_entity_type = std::string(entity_type);
}
