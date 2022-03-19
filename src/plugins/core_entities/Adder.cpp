#include <algorithm>
#include "Adder.h"

using namespace showtime;

Adder::Adder(const char * name) :
	ZstComputeComponent(ADDITION_FILTER_TYPE, name),
	m_augend(std::make_unique<ZstInputPlug>("addend", ZstValueType::FloatList)),
	m_addend(std::make_unique<ZstInputPlug>("augend", ZstValueType::FloatList)),
	m_sum(std::make_unique<ZstOutputPlug>("sum", ZstValueType::FloatList))
{
}

void Adder::on_registered()
{
	add_child(m_addend.get());
	add_child(m_augend.get());
	add_child(m_sum.get());
}

void Adder::compute(ZstInputPlug * plug)
{
	ZstComputeComponent::compute(plug);
	auto largest_size = std::max(m_addend->size(), m_augend->size());

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
ZstInputPlug * Adder::augend()
{
	return m_augend.get();
}

ZstInputPlug * Adder::addend()
{
	return m_addend.get();
}

ZstOutputPlug * Adder::sum()
{
	return m_sum.get();
}
