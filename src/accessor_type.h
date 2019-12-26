#ifndef ACCESSOR_TYPE_H
#define ACCESSOR_TYPE_H

#include <type_traits>

namespace celerity::algorithm
{
	enum class access_type
	{
		one_to_one,
		slice,
		chunk,
		item,
		invalid,
	};
}

namespace celerity::algorithm::detail
{

template <typename T, typename = std::void_t<>>
struct has_call_operator : std::false_type
{
};

template <typename T>
struct has_call_operator<T, std::void_t<decltype(&T::operator())>> : std::true_type
{
};

template <class T>
constexpr inline bool has_call_operator_v = has_call_operator<T>::value;

template <typename T>
struct function_traits
	: function_traits<decltype(&T::operator())>
{
};

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType (ClassType::*)(Args...) const>
{
	static constexpr auto arity = sizeof...(Args);

	using return_type = ReturnType;

	template <size_t I>
	struct arg
	{
		using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
	};
};

template <typename T, int I, bool has_call_operator>
struct arg_type;

template <typename T, int I>
struct arg_type<T, I, true>
{
	using type = typename function_traits<decltype(&T::operator())>::template arg<I>::type;
};

template <typename T, int I>
using arg_type_t = typename arg_type<T, I, has_call_operator_v<T>>::type;

template <typename ArgType, typename ElementType>
struct accessor_type
{
	using type = std::conditional_t<std::is_same_v<ArgType, ElementType>, one_to_one, ArgType>;
};

template <typename F, int I, typename ElementType>
using accessor_type_t = typename accessor_type<std::decay_t<arg_type_t<F, I>>, ElementType>::type;

template <typename ArgType>
constexpr access_type get_accessor_type_()
{
	using decayed_type = std::decay_t<ArgType>;

	if constexpr (detail::is_slice_v<decayed_type>)
	{
		return access_type::slice;
	}
	else if constexpr (detail::is_chunk_v<decayed_type>)
	{
		return access_type::chunk;
	}
	else if constexpr (detail::is_item_v<decayed_type>)
	{
		return access_type::item;
	}
	else
	{
		return access_type::one_to_one;
	}
}

template <typename F, int I>
constexpr bool in_bounds()
{
	return function_traits<F>::arity < I;
}

template <typename F, int I>
constexpr std::enable_if_t<has_call_operator_v<F>, access_type> get_accessor_type()
{
	return get_accessor_type_<arg_type_t<F, I>>();
}

template <typename F, int>
constexpr std::enable_if_t<!has_call_operator_v<F>, access_type> get_accessor_type()
{
	static_assert(std::is_void_v<F>, "invalid functor");
	return access_type::invalid;
}

} // namespace celerity::algorithm::detail

#endif