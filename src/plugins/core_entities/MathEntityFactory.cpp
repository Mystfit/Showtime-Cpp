#include "MathEntityFactory.h"

MathEntityFactory::MathEntityFactory(const char* name) : showtime::ZstEntityFactory(name)
{
	this->add_creatable<Adder>("adder");
}
