#ifndef SEQUENCING_H
#define SEQUENCING_H

#include "depth_first.h"
#include "fusion.h"
#include "linkage.h"
#include "require.h"
#include "subrange.h"

namespace celerity
{
	template <typename T, int Rank, typename U,
			  hla::require_one<hla::traits::is_linkable_sink_v<U>,
							   hla::traits::is_iterator_transform_v<U>> = hla::yes>
	auto operator|(celerity::buffer<T, Rank> lhs, U rhs)
	{
		return hla::detail::sequence(lhs, rhs);
	}

	template <typename T, int Rank, typename U, hla::require<hla::traits::is_linkable_sink_v<T>> = hla::yes>
	auto operator<<(T lhs, celerity::buffer<U, Rank> rhs)
	{
		using namespace hla::detail;
		return partial_t_joint{lhs, sequence(rhs)};
	}

	template <typename T, int Rank, typename U,
			  hla::require<!hla::traits::is_sequence_v<T>> = hla::yes>
	auto operator|(T lhs, celerity::buffer<U, Rank> rhs)
	{
		return hla::detail::sequence(lhs, rhs);
	}

	template <typename T, int Rank, typename U,
			  hla::require<hla::traits::is_sequence_v<T>> = hla::yes>
	auto operator|(T lhs, celerity::buffer<U, Rank> rhs)
	{
		return hla::detail::apply_append(lhs, rhs, [](auto &&a, auto &&b) { return operator|(std::forward<decltype(a)>(a), std::forward<decltype(b)>(b)); });
	}

} // namespace celerity

namespace celerity::hla
{
	template <typename T, typename U, require<traits::is_partially_packaged_task_v<T>, traits::is_partially_packaged_task_v<U>> = yes>
	auto operator|(T lhs, U rhs)
	{
		return hla::detail::sequence(lhs, rhs);
	}

	template <typename T, typename U, require<traits::is_sequence_v<T>, traits::is_partially_packaged_task_v<U>> = yes>
	auto operator|(T lhs, U rhs)
	{
		return lhs | hla::detail::sequence(rhs);
	}

	template <int Rank>
	auto operator|(hla::detail::iterator_transform<Rank> lhs, hla::detail::iterator_transform<Rank> rhs)
	{
		return hla::detail::sequence(lhs, rhs);
	}

	template <typename T, typename U, require<traits::is_sequence_v<T>, traits::is_iterator_transform_v<traits::last_element_t<T>>, traits::is_iterator_transform_v<U>> = yes>
	auto operator|(T lhs, U rhs)
	{
		return lhs | hla::detail::sequence(rhs);
	}

	template <typename T, typename U, require<traits::is_sequence_v<T>, traits::is_sequence_v<U>> = yes>
	auto operator|(T lhs, U rhs)
	{
		return lhs | rhs;
	}

	template <typename T, typename U, require<traits::is_sequence_v<T>, traits::is_sequence_v<U>> = yes>
	auto operator<<(T lhs, U rhs)
	{
		apply_append(lhs, rhs, [](auto &&a, auto &&b) { return hla::detail::partial_t_joint{std::forward<decltype(a)>(a), std::forward<decltype(b)>(b)}; });
	}

	template <typename T, typename U, require<traits::is_sequence_v<T>, traits::is_linkable_source_v<U>> = yes>
	auto operator<<(T lhs, U rhs)
	{
		apply_append(lhs, rhs, [](auto &&a, auto &&b) { return hla::detail::partial_t_joint{std::forward<decltype(a)>(a), std::forward<decltype(b)>(b)}; });
	}

	template <typename T, typename U, require<traits::is_linkable_sink_v<T>, traits::is_sequence_v<U>> = yes>
	auto operator<<(T lhs, U rhs)
	{
		using namespace hla::detail;
		return partial_t_joint(lhs, rhs);
	}

	template <typename T, typename U, require<traits::is_linkable_sink_v<T>, traits::is_linkable_source_v<U>> = yes>
	auto operator<<(T lhs, U rhs)
	{
		using namespace hla::detail;
		return partial_t_joint(lhs, sequence(rhs));
	}

	template <typename T, require_one<traits::is_packaged_task_v<T>, traits::is_packaged_task_sequence_v<T>> = yes>
	auto operator|(T lhs, distr_queue q)
	{
		using namespace traits;

		if constexpr (is_packaged_task_v<T> || (is_packaged_task_sequence_v<T> && size_v<T> == 1))
		{
			return std::invoke(lhs, q);
		}
		else
		{
			auto r = std::invoke(lhs, q);
			return std::get<std::tuple_size_v<decltype(r)> - 1>(r);
		}
	}

	template <typename T, require<!traits::is_packaged_task_v<T>, !traits::is_packaged_task_sequence_v<T>> = yes>
	auto operator|(T lhs, distr_queue q)
	{
		using namespace detail;
		auto s = lhs | seq::end;

		static_assert(traits::is_packaged_task_v<decltype(s)> || traits::is_packaged_task_sequence_v<decltype(s)>);

		return s | q;
	}

	inline auto submit_to(celerity::distr_queue q)
	{
		return q;
	}

	template <typename T>
	using sequence_t = std::conditional_t<traits::is_sequence_v<T>, T, decltype(hla::detail::sequence(std::declval<T>()))>;

	template <typename T>
	using resolved_t = decltype(resolve_subranges(std::declval<sequence_t<T>>()));

	template <typename T>
	using linked_t = decltype(link(std::declval<resolved_t<T>>()));

	template <typename T>
	using terminated_t = decltype(terminate(std::declval<linked_t<T>>()));

	template <typename T>
	using fused_t = decltype(fuse(std::declval<terminated_t<T>>()));

} // namespace celerity::hla

#endif // SEQUENCING_H