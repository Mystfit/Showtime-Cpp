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
        ZST_EXPORT ZstBundleIterator(typename std::vector<T>::iterator it) : m_it(it){
        }
        
        ZST_EXPORT bool operator!=(const ZstBundleIterator& other){
            return (m_it != other.m_it);
        }
        
        ZST_EXPORT bool operator==(const ZstBundleIterator& other){
            return (m_it == other.m_it);
        }
        
        ZST_EXPORT T operator*() const{
            return *m_it;
        }
        
        ZST_EXPORT const ZstBundleIterator& operator++(){
            m_it++;
            return *this;
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
        ZST_EXPORT ZstBundle(){};
        ZST_EXPORT ~ZstBundle(){};
        ZST_EXPORT ZstBundle(const ZstBundle& other){
            m_bundle_items = other.m_bundle_items;
        };
        ZST_EXPORT ZstBundle(ZstBundle&& other){
            m_bundle_items = std::move(other.m_bundle_items);
            other.m_bundle_items.clear();
        }

        ZST_EXPORT ZstBundleIterator<T> begin(){
            return ZstBundleIterator<T>(m_bundle_items.begin());
        }
        ZST_EXPORT ZstBundleIterator<T> end(){
            return ZstBundleIterator<T>(m_bundle_items.end());
        }
        ZST_EXPORT void add(T bundle_item){
            m_bundle_items.emplace_back(bundle_item);
        }

        ZST_EXPORT T item_at(const size_t index) const{
            if (index >= m_bundle_items.size()) {
                throw std::out_of_range("Bundle index out of range");
            }
            return m_bundle_items[index];
        }

        ZST_EXPORT size_t size() const{
            return m_bundle_items.size();
        }
        ZST_EXPORT T operator[](const size_t& index) const{
            return item_at(index);
        }
        ZST_EXPORT void clear(){
            m_bundle_items.clear();
        }

	private:
		typename std::vector<T> m_bundle_items;
	};
}
