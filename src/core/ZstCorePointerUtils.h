#pragma once

#include <boost/shared_ptr.hpp>
#include <memory>

template<typename T>
boost::shared_ptr<T> to_boost_shared_ptr(std::shared_ptr<T>& ptr)
{
	return boost::shared_ptr<T>(ptr.get(), [ptr](T*) mutable {ptr.reset(); });
}

template<typename T>
std::shared_ptr<T> from_boost_shared_ptr(boost::shared_ptr<T>& ptr)
{
	return std::shared_ptr<T>(ptr.get(), [ptr](T*) mutable {ptr.reset(); });
}
