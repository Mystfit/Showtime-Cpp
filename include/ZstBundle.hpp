#pragma once

#include <ZstExports.h>

//---------------------
//Bundle iterator
//---------------------
template<typename T>
class ZstBundleIterator 
{
public:
	ZST_EXPORT ZstBundleIterator::ZstBundleIterator(typename std::vector<T>::iterator it) :
		m_it(it)
	{
	}

	ZST_EXPORT bool ZstBundleIterator::operator!=(const ZstBundleIterator & other)
	{
		return (m_it != other.m_it);
	}

	ZST_EXPORT const ZstBundleIterator & ZstBundleIterator::operator++()
	{
		m_it++;
		return *this;
	}

	ZST_EXPORT T ZstBundleIterator::operator*() const
	{
		return *m_it;
	}

private:
	typename std::vector<T>::iterator m_it;
};


//---------------------
//Bundle
//---------------------

template<typename T>
class ZstBundle
{
public:
	ZST_EXPORT ZstBundle::ZstBundle()
	{
	}

	ZST_EXPORT ZstBundle::~ZstBundle()
	{
	}

	ZST_EXPORT ZstBundleIterator<T> ZstBundle::begin()
	{
		return ZstBundleIterator<T>(m_bundle_items.begin());
	}

	ZST_EXPORT ZstBundleIterator<T> ZstBundle::end()
	{
		return ZstBundleIterator<T>(m_bundle_items.end());
	}

	ZST_EXPORT void ZstBundle::add(T bundle_item)
	{
		m_bundle_items.push_back(bundle_item);
	}

	ZST_EXPORT T ZstBundle::item_at(size_t index)
	{
		return m_bundle_items[index];
	}

	ZST_EXPORT size_t ZstBundle::size()
	{
		return m_bundle_items.size();
	}

private:
	typename std::vector<T> m_bundle_items;
};

