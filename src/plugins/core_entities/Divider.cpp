#include <algorithm>
#include "Divider.h"

using namespace showtime;

Divider::Divider(const char * name) :
	ZstComputeComponent(DIVISION_FILTER_TYPE, name),
	m_dividend(std::make_unique<ZstInputPlug>("dividend", ZstValueType::FloatList)),
	m_divisor(std::make_unique<ZstInputPlug>("divisor", ZstValueType::FloatList)),
	m_quotient(std::make_unique<ZstOutputPlug>("quotient", ZstValueType::FloatList))
{
}

void Divider::on_registered()
{
	add_child(m_dividend.get());
	add_child(m_divisor.get());
	add_child(m_quotient.get());
}

void Divider::compute(ZstInputPlug * plug)
{
	ZstComputeComponent::compute(plug);
	auto largest_size = std::max(m_dividend->size(), m_divisor->size());

	for (int i = 0; i < largest_size; ++i) {
		if (m_dividend->size() > i && m_divisor->size() > i) {
			float divisor_value = m_divisor->float_at(i);
			if (divisor_value != 0.0f) {
				float result = m_dividend->float_at(i) / divisor_value;
				m_quotient->append_float(result);
			}
			else {
				// Handle division by zero by outputting 0
				m_quotient->append_float(0.0f);
			}
		}
		else if (m_dividend->size() > i) {
			m_quotient->append_float(m_dividend->float_at(i));
		}
		else if (m_divisor->size() > i) {
			float divisor_value = m_divisor->float_at(i);
			if (divisor_value != 0.0f) {
				m_quotient->append_float(0.0f);
			}
			else {
				m_quotient->append_float(0.0f);
			}
		}
	}
	m_quotient->fire();
}

// --------------
// Plug accessors
// --------------
ZstInputPlug * Divider::dividend()
{
	return m_dividend.get();
}

ZstInputPlug * Divider::divisor()
{
	return m_divisor.get();
}

ZstOutputPlug * Divider::quotient()
{
	return m_quotient.get();
}
