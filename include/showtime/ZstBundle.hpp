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
        ZstBundleIterator(typename std::vector<T>::iterator it) : m_it(it){
        }
        
        bool operator!=(const ZstBundleIterator& other){
            return (m_it != other.m_it);
        }
        
        bool operator==(const ZstBundleIterator& other){
            return (m_it == other.m_it);
        }
        
        T operator*() const{
            return *m_it;
        }
        
        const ZstBundleIterator& operator++(){
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
        ZstBundle(){};
        ~ZstBundle(){};
        ZstBundle(const ZstBundle& other){
            m_bundle_items = other.m_bundle_items;
        };
        ZstBundle(ZstBundle&& other){
            m_bundle_items = std::move(other.m_bundle_items);
            other.m_bundle_items.clear();
        }

        ZstBundleIterator<T> begin(){
            return ZstBundleIterator<T>(m_bundle_items.begin());
        }
        ZstBundleIterator<T> end(){
            return ZstBundleIterator<T>(m_bundle_items.end());
        }
        void add(T bundle_item){
            m_bundle_items.emplace_back(bundle_item);
        }

        T item_at(const size_t index) const{
            if (index >= m_bundle_items.size()) {
                throw std::out_of_range("Bundle index out of range");
            }
            return m_bundle_items[index];
        }

        size_t size() const{
            return m_bundle_items.size();
        }
        T operator[](const size_t& index) const{
            return item_at(index);
        }
        void clear(){
            m_bundle_items.clear();
        }

	private:
		typename std::vector<T> m_bundle_items;
	};
}
