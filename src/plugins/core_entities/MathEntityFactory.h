#pragma once

#include "ZstExports.h"
#include "entities/ZstEntityFactory.h"
#include "Adder.h"


class ZST_CLASS_EXPORTED MathEntityFactory : public showtime::ZstEntityFactory
{
public:
	MathEntityFactory(const char* name) : showtime::ZstEntityFactory(name)
	{
		this->add_creatable<Adder>("adder");
	}
};