#ifndef T_JOINT_H
#define T_JOINT_H

#include "packaged_task_traits.h"

namespace celerity::algorithm
{

template <typename Task, typename SecondaryInputSequence>
struct t_joint
{
public:
    t_joint(Task task, SecondaryInputSequence sequence)
        : task_(task), secondary_in_(sequence)
    {
    }

    auto operator()(celerity::distr_queue &queue) const
    {
        std::invoke(secondary_in_, queue);
        return std::invoke(task_, queue);
    }

    auto get_in_beg() const { return task_.get_in_beg(); }
    auto get_in_end() const { return task_.get_in_end(); }
    auto get_out_iterator() const { return task_.get_out_iterator(); }
    auto get_range() const { return task_.get_range(); }

    auto get_task() { return task_; }
    auto get_secondary() { return secondary_in_; }

private:
    Task task_;
    SecondaryInputSequence secondary_in_;
};

template <typename Task, typename SecondaryInputSequence>
struct partial_t_joint
{
public:
    partial_t_joint(Task task, SecondaryInputSequence sequence)
        : task_(task), secondary_in_(sequence)
    {
    }

    template <typename IteratorType>
    auto complete(IteratorType beg, IteratorType end)
    {
        auto completed_task = task_.complete(beg, end);

        using completed_task_type = decltype(completed_task);

        if constexpr (detail::is_partially_packaged_task_v<completed_task_type>)
        {
            return partial_t_joint<completed_task_type, SecondaryInputSequence>{
                completed_task, secondary_in_};
        }
        else
        {
            return t_joint<completed_task_type, SecondaryInputSequence>{
                completed_task, secondary_in_};
        }
    }

    auto get_in_beg() const { return task_.get_in_beg(); }
    auto get_in_end() const { return task_.get_in_end(); }
    auto get_range() const { return task_.get_range(); }

private:
    Task task_;
    SecondaryInputSequence secondary_in_;
};

namespace detail
{

template <typename Task, typename SecondaryInputSequence>
struct is_packaged_task<t_joint<Task, SecondaryInputSequence>>
    : std::bool_constant<true>
{
};

template <typename Task, typename SecondaryInputSequence>
struct packaged_task_traits<t_joint<Task, SecondaryInputSequence>>
{
    using traits = packaged_task_traits<Task>;

    static constexpr auto rank = traits::rank;
    static constexpr auto computation_type = traits::computation_type;
    static constexpr auto access_type = traits::access_type;

    using input_iterator_type = typename traits::input_iterator_type;
    using input_value_type = typename traits::input_value_type;
    using output_value_type = typename traits::output_value_type;
    using output_iterator_type = typename traits::output_iterator_type;
};

template <typename Task, typename SecondaryInputSequence>
struct extended_packaged_task_traits<t_joint<Task, SecondaryInputSequence>, computation_type::zip>
    : extended_packaged_task_traits<Task, computation_type::zip>
{
};

template <typename Task, typename SecondaryInputSequence>
struct is_partially_packaged_task<partial_t_joint<Task, SecondaryInputSequence>>
    : std::bool_constant<true>
{
};

template <typename Task, typename SecondaryInputSequence>
struct packaged_task_traits<partial_t_joint<Task, SecondaryInputSequence>>
{
    using traits = packaged_task_traits<Task>;

    static constexpr auto rank = traits::rank;
    static constexpr auto computation_type = traits::computation_type;
    static constexpr auto access_type = traits::access_type;

    using input_iterator_type = typename traits::input_iterator_type;
    using input_value_type = typename traits::input_value_type;
    using output_value_type = typename traits::output_value_type;
    using output_iterator_type = typename traits::output_iterator_type;
};

template <typename Task, typename SecondaryInputSequence>
struct extended_packaged_task_traits<partial_t_joint<Task, SecondaryInputSequence>, computation_type::zip>
    : extended_packaged_task_traits<Task, computation_type::zip>
{
};

template <typename Task, typename SecondaryInputSequence>
struct partially_packaged_task_traits<partial_t_joint<Task, SecondaryInputSequence>>
    : partially_packaged_task_traits<Task>
{
};

template <typename T>
struct is_t_joint : std::bool_constant<false>
{
};

template <typename Task, typename SecondaryInputSequence>
struct is_t_joint<t_joint<Task, SecondaryInputSequence>>
    : std::bool_constant<true>
{
};

template <typename Task, typename SecondaryInputSequence>
struct is_t_joint<partial_t_joint<Task, SecondaryInputSequence>>
    : std::bool_constant<true>
{
};

template <typename T>
constexpr inline bool is_t_joint_v = is_t_joint<T>::value;

template <typename T>
struct t_joint_traits
{
    using task_type = void;
    using secondary_input_sequence_type = sequence<>;
};

template <typename Task, typename SecondaryInputSequence>
struct t_joint_traits<t_joint<Task, SecondaryInputSequence>>
{
    using task_type = Task;
    using secondary_input_sequence_type = SecondaryInputSequence;
};

} // namespace detail

} // namespace celerity::algorithm

#endif // T_JOINT_H