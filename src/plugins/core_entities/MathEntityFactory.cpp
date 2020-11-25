#include "MathEntityFactory.h"

MathEntityFactory::MathEntityFactory(const char* name) : showtime::ZstEntityFactory(name)
{
	this->add_creatable("adder", [](const char* name) {return std::make_unique<Adder>(name); });
}
