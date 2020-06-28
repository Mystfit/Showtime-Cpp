#pragma once

#include <showtime/ZstExports.h>

#if defined(_LIBCPP_STD_VER) && (_LIBCPP_STD_VER < 17)
namespace showtime {
	// Backported enable_shared_from_this class with weak_from_this support
	template<class _Tp>
	class _LIBCPP_TEMPLATE_VIS enable_shared_from_this
	{
		mutable std::weak_ptr<_Tp> __weak_this_;
	protected:
		_LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR
			enable_shared_from_this() _NOEXCEPT {}
		_LIBCPP_INLINE_VISIBILITY
			enable_shared_from_this(enable_shared_from_this const&) _NOEXCEPT {}
		_LIBCPP_INLINE_VISIBILITY
			enable_shared_from_this& operator=(enable_shared_from_this const&) _NOEXCEPT
		{
			return *this;
		}
		_LIBCPP_INLINE_VISIBILITY
			~enable_shared_from_this() {}
	public:
		_LIBCPP_INLINE_VISIBILITY
			std::shared_ptr<_Tp> shared_from_this()
		{
			return std::shared_ptr<_Tp>(__weak_this_);
		}
		_LIBCPP_INLINE_VISIBILITY
			std::shared_ptr<_Tp const> shared_from_this() const
		{
			return std::shared_ptr<const _Tp>(__weak_this_);
		}

		_LIBCPP_INLINE_VISIBILITY
			std::weak_ptr<_Tp> weak_from_this() _NOEXCEPT
		{
			return __weak_this_;
		}

		_LIBCPP_INLINE_VISIBILITY
			std::weak_ptr<const _Tp> weak_from_this() const _NOEXCEPT
		{
			return __weak_this_;
		}

		template <class _Up> friend class std::shared_ptr;
	};
}
class ZST_CLASS_EXPORTED MultipleInheritableEnableSharedFromThis : public showtime::enable_shared_from_this<MultipleInheritableEnableSharedFromThis>
#else
#include <memory>
class ZST_CLASS_EXPORTED MultipleInheritableEnableSharedFromThis : public std::enable_shared_from_this<MultipleInheritableEnableSharedFromThis>
#endif
{
public:
	std::shared_ptr<MultipleInheritableEnableSharedFromThis> get_shared_ptr() {
		return this->shared_from_this();
	}

	std::weak_ptr<MultipleInheritableEnableSharedFromThis> get_weak_ptr() {
		return this->weak_from_this();
	}

	virtual ~MultipleInheritableEnableSharedFromThis() {}
};

template <class T>
class ZST_CLASS_EXPORTED inheritable_enable_shared_from_this : virtual public MultipleInheritableEnableSharedFromThis
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

	template <class Derived>
	std::shared_ptr<Derived> shared_from_base()
	{
		return std::static_pointer_cast<Derived>(shared_from_this());
	}
};

