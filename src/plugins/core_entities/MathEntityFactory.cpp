#include "MathEntityFactory.h"
#include "Adder.h"
#include "Multiplier.h"
#include "Subtractor.h"
#include "Divider.h"

MathEntityFactory::MathEntityFactory(const char* name) : showtime::ZstEntityFactory(name)
{
	this->add_creatable("adder", [](const char* name) {return std::make_unique<Adder>(name); });
	this->add_creatable("multiplier", [](const char* name) {return std::make_unique<Multiplier>(name); });
	this->add_creatable("subtractor", [](const char* name) {return std::make_unique<Subtractor>(name); });
	this->add_creatable("divider", [](const char* name) {return std::make_unique<Divider>(name); });
}
