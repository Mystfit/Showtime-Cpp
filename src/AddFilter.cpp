#include "entities/AddFilter.h"
#include "Showtime.h"

using namespace std;

AddFilter::AddFilter(ZstEntityBase * parent) :
	ZstFilter(ADDITION_FILTER_TYPE, "add", parent)
{
	init();
}

void AddFilter::init()
{
	if (is_registered()) {
		m_addend = create_input_plug("addend", ZstValueType::ZST_FLOAT);
		m_augend = create_input_plug("augend", ZstValueType::ZST_FLOAT);
		m_sum = create_output_plug("sum", ZstValueType::ZST_FLOAT);
	}
}

void AddFilter::compute(ZstInputPlug * plug)
{
	m_sum->value().clear();
	int largest_size = (m_addend->value().size() > m_augend->value().size()) ? m_addend->value().size() : m_augend->value().size();
	
	for (int i = 0; i < largest_size; ++i) {
		if (m_augend->value().size() > i && m_addend->value().size() > i) {
			m_sum->value().append_float(m_augend->value().float_at(i) + m_addend->value().float_at(i));
		}
		else if (m_augend->value().size() > i) {
			m_sum->value().append_float(m_augend->value().float_at(i));
		}
		else if (m_addend->value().size() > i) {
			m_sum->value().append_float(m_addend->value().float_at(i));
		}
	}
	m_sum->fire();
}

// --------------
// Plug accessors
// --------------
ZstInputPlug * AddFilter::augend()
{
	return m_augend;
}

ZstInputPlug * AddFilter::addend()
{
	return m_addend;
}

ZstOutputPlug * AddFilter::sum()
{
	return m_sum;
}
