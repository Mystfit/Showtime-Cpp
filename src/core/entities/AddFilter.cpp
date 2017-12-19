#include "entities/AddFilter.h"
#include "ZstConstants.h"

using namespace std;

AddFilter::AddFilter(const char * name) :
	ZstComponent(ADDITION_FILTER_TYPE, name)
{
	m_addend = create_input_plug("addend", ZstValueType::ZST_FLOAT);
	m_augend = create_input_plug("augend", ZstValueType::ZST_FLOAT);
	m_sum = create_output_plug("sum", ZstValueType::ZST_FLOAT);
}

void AddFilter::init()
{
}

void AddFilter::compute(ZstInputPlug * plug)
{
	m_sum->clear();
	int largest_size = (m_addend->size() > m_augend->size()) ? m_addend->size() : m_augend->size();
	
	for (int i = 0; i < largest_size; ++i) {
		if (m_augend->size() > i && m_addend->size() > i) {
			float result = m_augend->float_at(i) + m_addend->float_at(i);
			m_sum->append_float(result);
		}
		else if (m_augend->size() > i) {
			m_sum->append_float(m_augend->float_at(i));
		}
		else if (m_addend->size() > i) {
			m_sum->append_float(m_addend->float_at(i));
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
