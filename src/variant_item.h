#ifndef VARIANT_ITEM
#define VARIANT_ITEM

#include "sycl.h"

#include <variant>

template <int... Ranks>
class variant_item
{
public:
    using variant_type = std::variant<
        cl::sycl::item<Ranks>...>;

    template <int Rank>
    explicit variant_item(cl::sycl::item<Rank> item)
        : var_(item), rank_(Rank) {}

    template <typename F>
    void apply(const F &f) const
    {
        switch (rank_)
        {
        case 1:
            if constexpr (((Ranks == 1) || ...))
                f(*std::get_if<cl::sycl::item<1>>(&var_));
            break;
        case 2:
            if constexpr (((Ranks == 2) || ...))
                f(*std::get_if<cl::sycl::item<2>>(&var_));
            break;
        case 3:
            if constexpr (((Ranks == 3) || ...))
                f(*std::get_if<cl::sycl::item<3>>(&var_));
            break;
        }
    }

private:
    const int rank_;
    const variant_type var_;
};

#endif