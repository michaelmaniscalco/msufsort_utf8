#pragma once

#include <include/convertible_to_span_of_bytes.h>

#include <vector>
#include <span>
#include <cstdint>


namespace maniscalco
{

    //=========================================================================
    std::vector<std::uint32_t> make_suffix_array
    (
        std::span<std::byte const>
    );


    //=========================================================================
    template <typename T>
    std::vector<std::uint32_t> make_suffix_array
    (
        T source
    ) requires convertible_to_span_of_bytes<T>
    {
        return make_suffix_array(std::as_bytes(std::span(source.begin(), source.size())));
    }


    //=========================================================================
    std::vector<std::byte> make_burrows_wheeler_transform
    (
        std::span<std::byte const>
    );


    //=========================================================================
    template <typename T>
    std::vector<std::byte> make_burrows_wheeler_transform
    (
        T source
    ) requires convertible_to_span_of_bytes<T>
    {
        return make_burrows_wheeler_transform(std::as_bytes(std::span(source.begin(), source.size())));
    }


    std::vector<std::byte> reverse_burrows_wheeler_transform
    (
        std::span<std::byte const>,
        std::uint32_t,
        std::uint32_t
    );

} // namespace maniscalco