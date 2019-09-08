#ifndef ACCESSOR_PROXY_H
#define ACCESSOR_PROXY_H

#include "celerity.h"
#include "accessors.h"

#include <type_traits>
#include <cmath>

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

	namespace detail
	{
		template<typename T, typename = std::void_t<>>
		struct has_call_operator : std::false_type {};

		template<typename T>
		struct has_call_operator<T, std::void_t<decltype(&T::operator())>> : std::true_type {};

		template<class T>
		constexpr inline bool has_call_operator_v = has_call_operator<T>::value;
			   		
		template <typename T>
		struct function_traits
			: function_traits<decltype(&T::operator())>
		{
		};

		template <typename ClassType, typename ReturnType, typename... Args>
		struct function_traits<ReturnType(ClassType::*)(Args...) const>
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

		template<typename T, int I>
		struct arg_type<T, I, true>
		{
			using type = typename function_traits<decltype(&T::operator())>::template arg<I>::type;
		};

		template<typename T, int I>
		using arg_type_t = typename arg_type<T, I, has_call_operator_v<T>>::type;

		template <typename ArgType, typename ElementType>
		struct accessor_type
		{
			using type = std::conditional_t<std::is_same_v<ArgType, ElementType>, one_to_one, ArgType>;
		};

		template <typename F, int I, typename ElementType>
		using accessor_type_t = typename accessor_type<arg_type_t<F, I>, ElementType>::type;

		template<typename ArgType>
		constexpr access_type get_accessor_type_()
		{
			if constexpr (is_slice_v<ArgType>)
			{
				return access_type::slice;
			}
			else if constexpr (is_chunk_v<ArgType>)
			{
				return access_type::chunk;
			}
			else if constexpr (is_item_v<ArgType>)
			{
				return access_type::item;
			}
			else
			{
				return access_type::one_to_one;
			}
		}

		template<typename F, int I>
		constexpr bool in_bounds()
		{
			return function_traits<F>::arity < I;
		}
		
		template<typename F, int I>
		constexpr std::enable_if_t<has_call_operator_v<F>, access_type> get_accessor_type()
		{
			using traits = function_traits<F>;			
			return get_accessor_type_<arg_type_t<F, I>>();
		}

		template<typename F, int>
		constexpr std::enable_if_t<!has_call_operator_v<F>, access_type> get_accessor_type()
		{
			return access_type::invalid;
		}
	}

	template<typename T, size_t Rank, typename AccessorType, typename Type>
	class accessor_proxy;

	template<typename T, size_t Rank, typename AccessorType>
	class accessor_proxy<T, Rank, AccessorType, one_to_one>
	{
	public:
		explicit accessor_proxy(AccessorType acc) : accessor_(acc) {}

		T operator[](const cl::sycl::item<Rank> item) const { return accessor_[item]; }
		T& operator[](const cl::sycl::item<Rank> item) { return accessor_[item]; }

		AccessorType& get_accessor() { return accessor_; }
		
	private:
		AccessorType accessor_;
	};

	template<typename T, size_t Rank, typename AccessorType, size_t Dim>
	class accessor_proxy<T, Rank, AccessorType, slice<T, Dim>>
	{
	public:
		static_assert(Dim >= 0 && Dim < Rank, "Dim out of bounds");

		using getter_t = detail::slice_element_getter_t<T>;

		explicit accessor_proxy(AccessorType acc) 
			: accessor_(acc) {}

		slice<T, Dim> operator[](const cl::sycl::item<Rank> it)
		{
			getter_ = [this, it](int i)
			{ 
				auto id = it;

				id[Dim] = i;

				return accessor_[id];
			};

			return slice<T, Dim>{ static_cast<int>(it.get_id()[Dim]), getter_ };
		}

	private:
		getter_t getter_;
		AccessorType accessor_;
	};

	template<typename T, size_t Rank, typename AccessorType, size_t...Extents>
	class accessor_proxy<T, Rank, AccessorType, chunk<T, Extents...>>
	{
	public:
		explicit accessor_proxy(AccessorType acc) : accessor_(acc) {}

		chunk<T, Extents...> operator[](const cl::sycl::item<Rank> item) const
		{
			return { item, [this, item](cl::sycl::rel_id<Rank> rel_id)
			{
				cl::sycl::id<Rank> id;
				for (auto i = 0; i < Rank; ++i)
				{
					id[i] = std::max(0l, std::min(static_cast<long>(item.get_id()[i]) + rel_id[i], static_cast<long>(item.get_range()[i]) - 1));
				}
				
				return accessor_[{item.get_range(), id}];
			} };
		}

	private:
		AccessorType accessor_;
	};

	template<celerity::access_mode Mode, typename AccessorType, typename T, size_t Rank>
	auto get_access(celerity::handler cgh, buffer_iterator<T, Rank> beg, buffer_iterator<T, Rank> end)
	{
		//assert(&beg.buffer() == &end.buffer());
		//assert(*beg <= *end);

		auto acc = beg.buffer().template get_access<Mode>(cgh, distance(beg, end), accessor_traits<AccessorType>::range_mapper());

		return accessor_proxy<T, Rank, decltype(acc), AccessorType>{ acc };
	}
}

#endif // ACCESSOR_PROXY_H

