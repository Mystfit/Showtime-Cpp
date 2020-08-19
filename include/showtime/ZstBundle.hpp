#pragma once
#include <vector>
#include <showtime/ZstExports.h>

//---------------------
//Bundle iterator
//---------------------
namespace showtime {
	template<typename T>
	class ZstBundleIterator
	{
	public:
		ZST_EXPORT ZstBundleIterator(typename std::vector<T>::iterator it);
		ZST_EXPORT bool operator!=(const ZstBundleIterator& other);
		ZST_EXPORT bool operator==(const ZstBundleIterator& other);
		ZST_EXPORT T operator*() const;
		ZST_EXPORT const ZstBundleIterator& operator++();

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
		ZST_EXPORT ZstBundle();
		ZST_EXPORT ~ZstBundle();
		ZST_EXPORT ZstBundle(const ZstBundle& other);

		ZST_EXPORT ZstBundleIterator<T> begin();
		ZST_EXPORT ZstBundleIterator<T> end();
		ZST_EXPORT void add(T bundle_item);

		ZST_EXPORT T item_at(const size_t index) const;

		ZST_EXPORT size_t size() const;
		ZST_EXPORT T operator[](const size_t& index) const;
		ZST_EXPORT void clear();

	private:
		typename std::vector<T> m_bundle_items;
	};
}