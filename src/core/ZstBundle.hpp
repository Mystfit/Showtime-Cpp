#pragma once

#include <vector>

//---------------------
//Bundle iterator
//---------------------
template<typename T>
class ZstBundleIterator {
public:
	ZstBundleIterator::ZstBundleIterator(std::vector<T>::iterator it) :
		m_it(it)
	{
	}

	bool ZstBundleIterator::operator!=(const ZstBundleIterator & other)
	{
		return (m_it != other.m_it);
	}

	const ZstBundleIterator & ZstBundleIterator::operator++()
	{
		m_it++;
		return *this;
	}

	T ZstBundleIterator::operator*() const
	{
		return *m_it;
	}
};


//---------------------
//Bundle
//---------------------

template<typename T>
class ZstBundle {
public:
	ZstBundle::ZstBundle()
	{
	}

	ZstBundle::~ZstBundle()
	{
		m_bundle_items.clear();
	}

	void ZstBundle::add(T bundle_item)
	{
		m_bundle_items.push_back(bundle_item);
	}

	T ZstBundle::item_at(size_t index)
	{
		return m_bundle_items[index];
	}

	size_t ZstBundle::size()
	{
		return m_bundle_items.size();
	}

	ZstBundleIterator ZstBundle::begin()
	{
		return ZstBundleIterator<T>(m_bundle_items.begin());
	}

	ZstBundleIterator ZstBundle::end()
	{
		return ZstBundleIterator<T>(m_bundle_items.end());
	}

private:
	std::vector<T> m_bundle_items;
};
