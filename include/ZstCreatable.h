#pragma once

#include <string>
#include <vector>
#include "ZstExports.h"

class ZstEntityBase;

//Creatable events
struct ZstCreatableRecipe {
    std::string performer;
    std::string entity_type;
    std::vector<std::string> valid_parent_types;
};

class ZstCreateableKitchen {
public:
    ZST_EXPORT ZstCreateableKitchen(const char * entity_type);
    ZST_EXPORT virtual void cook(const char * entity_name, ZstEntityBase * parent) = 0;
private:
    std::string m_entity_type;
};
