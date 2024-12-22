#include <algorithm>
#include "Multiplier.h"

using namespace showtime;

Multiplier::Multiplier(const char * name) :
	ZstComputeComponent(MULTIPLICATION_FILTER_TYPE, name),
	m_multiplicand(std::make_unique<ZstInputPlug>("multiplicand", ZstValueType::FloatList)),
	m_multiplier(std::make_unique<ZstInputPlug>("multiplier", ZstValueType::FloatList)),
	m_product(std::make_unique<ZstOutputPlug>("product", ZstValueType::FloatList))
{
}

void Multiplier::on_registered()
{
	add_child(m_multiplicand.get());
	add_child(m_multiplier.get());
	add_child(m_product.get());
}

void Multiplier::compute(ZstInputPlug * plug)
{
	ZstComputeComponent::compute(plug);
	auto largest_size = std::max(m_multiplicand->size(), m_multiplier->size());

	for (int i = 0; i < largest_size; ++i) {
		if (m_multiplicand->size() > i && m_multiplier->size() > i) {
			float result = m_multiplicand->float_at(i) * m_multiplier->float_at(i);
			m_product->append_float(result);
		}
		else if (m_multiplicand->size() > i) {
			m_product->append_float(m_multiplicand->float_at(i));
		}
		else if (m_multiplier->size() > i) {
			m_product->append_float(m_multiplier->float_at(i));
		}
	}
	m_product->fire();
}

// --------------
// Plug accessors
// --------------
ZstInputPlug * Multiplier::multiplicand()
{
	return m_multiplicand.get();
}

ZstInputPlug * Multiplier::multiplier()
{
	return m_multiplier.get();
}

ZstOutputPlug * Multiplier::product()
{
	return m_product.get();
}
