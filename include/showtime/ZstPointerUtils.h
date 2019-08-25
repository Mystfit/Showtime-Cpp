#pragma once

#include <memory>

class MultipleInheritableEnableSharedFromThis : public std::enable_shared_from_this<MultipleInheritableEnableSharedFromThis>
{
public:
	virtual ~MultipleInheritableEnableSharedFromThis()
	{}
};

template <class T>
class inheritable_enable_shared_from_this : virtual public MultipleInheritableEnableSharedFromThis
{
public:
	std::shared_ptr<T> shared_from_this() {
		return std::dynamic_pointer_cast<T>(MultipleInheritableEnableSharedFromThis::shared_from_this());
	}

	std::weak_ptr<T> weak_from_this() {
		return std::dynamic_pointer_cast<T>(MultipleInheritableEnableSharedFromThis::weak_from_this());
	}
	/* Utility method to easily downcast.
	 * Useful when a child doesn't inherit directly from enable_shared_from_this
	 * but wants to use the feature.
	 */
	template <class Down>
	std::shared_ptr<Down> downcasted_shared_from_this() {
		return std::dynamic_pointer_cast<Down>(MultipleInheritableEnableSharedFromThis::shared_from_this());
	}

	template <class Down>
	std::weak_ptr<Down> downcasted_weak_from_this() {
		return std::dynamic_pointer_cast<Down>(MultipleInheritableEnableSharedFromThis::weak_from_this());
	}
};
