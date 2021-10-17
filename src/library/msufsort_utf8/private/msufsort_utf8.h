#pragma once

#include <numeric>
#include <mutex>
#include <span>
#include <vector>


namespace maniscalco
{

    using suffix_index = std::uint32_t;
    using suffix_value = std::uint64_t;
    using symbol = std::byte;

    class suffix_state;


    class msufsort
    {
    public:

        struct configuration
        {
        };

        msufsort
        (
            configuration const &
        );

        std::vector<suffix_index> suffix_array
        (
            std::span<symbol const>
        );

    private:

        static auto constexpr insertion_sort_threshold = 16;

        struct stack_frame;

        suffix_value get_value
        (
            symbol const *,
            suffix_index
        ) const;

         suffix_value __attribute__ ((noinline)) get_value_over
        (
            symbol const *,
            suffix_index
        ) const;

        suffix_index * multikey_quicksort
        (
            std::span<symbol const>,
            std::span<suffix_index>,
            std::uint32_t
        );

        void multikey_insertion_sort
        (
            std::span<symbol const>,
            std::span<suffix_index>,
            std::int32_t
        );

        bool compare_suffixes
        (
            std::span<symbol const>,
            symbol const *,
            suffix_index indexA,
            suffix_index indexB
        ) const;

        std::vector<suffix_index> select_suffixes
        (
            std::span<symbol const>,
            std::array<std::uint32_t, 0x10000> & 
        );

        symbol const *                                  getValueEnd_;

        suffix_index *                                  sa_;

        suffix_index *                                  isa_;

    }; // class msufsort

} // namespace maniscalco::msufsort
