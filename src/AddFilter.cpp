#include "entities\AddFilter.h"

using namespace std;

AddFilter::AddFilter() : ZstEntityBase(ADDITION_FILTER_TYPE, "add", ZstURI())
{
}

AddFilter::AddFilter(ZstURI parent) : ZstEntityBase(ADDITION_FILTER_TYPE, "add", parent)
{
}

void AddFilter::init()
{
	ZstEntityBase::init();

	if (is_registered()) {
		create_input_plug("addend", ZstValueType::ZST_FLOAT);
		create_input_plug("augend", ZstValueType::ZST_FLOAT);
		create_output_plug("sum", ZstValueType::ZST_FLOAT);
	}
}

void AddFilter::compute(ZstInputPlug * plug)
{
	sum->value()->clear();
	int largest_size = (addend->value()->size() > augend->value()->size()) ? addend->value()->size() : augend->value()->size();
	
	for (int i = 0; i < largest_size; ++i) {
		if (augend->value()->size() > i && addend->value()->size() > i) {
			sum->value()->append_float(augend->value()->float_at(i) + addend->value()->float_at(i));
		}
		else if (augend->value()->size() > i) {
			sum->value()->append_float(augend->value()->float_at(i));
		}
		else if (addend->value()->size() > i) {
			sum->value()->append_float(addend->value()->float_at(i));
		}
	}
	sum->fire();
}
