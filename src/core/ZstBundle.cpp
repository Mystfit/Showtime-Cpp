#include <showtime/ZstBundle.hpp>
#include <showtime/ZstURI.h>
#include <showtime/ZstCable.h>
#include <showtime/ZstServerAddress.h>
#include <showtime/entities/ZstEntityBase.h>
#include <showtime/entities/ZstEntityFactory.h>

namespace showtime {
	template<typename T>
	ZstBundleIterator<T>::ZstBundleIterator(typename std::vector<T>::iterator it) :
		m_it(it)
	{
	}
	
	template<typename T>
	bool ZstBundleIterator<T>::operator!=(const ZstBundleIterator& other)
	{
		return (m_it != other.m_it);
	}
	
	template<typename T>
	bool ZstBundleIterator<T>::operator==(const ZstBundleIterator& other)
	{
		return (m_it == other.m_it);
	}
	
	template<typename T>
	const ZstBundleIterator<T>& ZstBundleIterator<T>::operator++()
	{
		m_it++;
		return *this;
	}
	
	template<typename T>
	T ZstBundleIterator<T>::operator*() const
	{
		return *m_it;
	}
	
	template<typename T>
	ZstBundle<T>::ZstBundle()
	{
	}
	
	template<typename T>
	ZstBundle<T>::~ZstBundle()
	{
	}
	
	template<typename T>
	ZstBundle<T>::ZstBundle(const ZstBundle& other)
	{
		m_bundle_items = other.m_bundle_items;
	}
	
	template<typename T>
	ZstBundleIterator<T> ZstBundle<T>::begin()
	{
		return ZstBundleIterator<T>(m_bundle_items.begin());
	}
	
	template<typename T>
	ZstBundleIterator<T> ZstBundle<T>::end()
	{
		return ZstBundleIterator<T>(m_bundle_items.end());
	}
	
	template<typename T>
	void ZstBundle<T>::add(T bundle_item)
	{
		m_bundle_items.emplace_back(bundle_item);
	}
	
	template<typename T>
	T ZstBundle<T>::item_at(const size_t index) const
	{
		if (index >= m_bundle_items.size()) {
			throw std::out_of_range("Bundle index out of range");
		}
		return m_bundle_items[index];
	}
	
	template<typename T>
	size_t ZstBundle<T>::size() const
	{
		return m_bundle_items.size();
	}
	
	template<typename T>
	T ZstBundle<T>::operator[](const size_t& index) const
	{
		return item_at(index);
	}
	
	template<typename T>
	void ZstBundle<T>::clear()
	{
		m_bundle_items.clear();
	}

	template class ZstBundle<ZstURI>;
	template class ZstBundle<ZstCable*>;
	template class ZstBundle<ZstServerAddress>;
	template class ZstBundle<ZstEntityBase*>;
	template class ZstBundle<ZstEntityFactory*>;
	template class ZstBundleIterator<ZstURI>;
	template class ZstBundleIterator<ZstCable*>;
	template class ZstBundleIterator<ZstServerAddress>;
	template class ZstBundleIterator<ZstEntityBase*>;
	template class ZstBundleIterator<ZstEntityFactory*>;
}

