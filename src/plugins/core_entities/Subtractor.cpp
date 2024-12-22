#include <algorithm>
#include "Subtractor.h"

using namespace showtime;

Subtractor::Subtractor(const char * name) :
	ZstComputeComponent(SUBTRACTION_FILTER_TYPE, name),
	m_minuend(std::make_unique<ZstInputPlug>("minuend", ZstValueType::FloatList)),
	m_subtrahend(std::make_unique<ZstInputPlug>("subtrahend", ZstValueType::FloatList)),
	m_difference(std::make_unique<ZstOutputPlug>("difference", ZstValueType::FloatList))
{
}

void Subtractor::on_registered()
{
	add_child(m_minuend.get());
	add_child(m_subtrahend.get());
	add_child(m_difference.get());
}

void Subtractor::compute(ZstInputPlug * plug)
{
	ZstComputeComponent::compute(plug);
	auto largest_size = std::max(m_minuend->size(), m_subtrahend->size());

	for (int i = 0; i < largest_size; ++i) {
		if (m_minuend->size() > i && m_subtrahend->size() > i) {
			float result = m_minuend->float_at(i) - m_subtrahend->float_at(i);
			m_difference->append_float(result);
		}
		else if (m_minuend->size() > i) {
			m_difference->append_float(m_minuend->float_at(i));
		}
		else if (m_subtrahend->size() > i) {
			m_difference->append_float(-m_subtrahend->float_at(i));
		}
	}
	m_difference->fire();
}

// --------------
// Plug accessors
// --------------
ZstInputPlug * Subtractor::minuend()
{
	return m_minuend.get();
}

ZstInputPlug * Subtractor::subtrahend()
{
	return m_subtrahend.get();
}

ZstOutputPlug * Subtractor::difference()
{
	return m_difference.get();
}
