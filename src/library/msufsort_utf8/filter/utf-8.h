#pragma once

#include <cstdint>
#include <span>
#include <vector>


namespace maniscalco
{

    std::vector<std::uint32_t> utf8_filter
    (
        std::span<std::uint8_t const>
    );

    std::pair<std::uint32_t, std::uint8_t>  utf8_decode
    (
        std::uint8_t const *
    );

    std::uint8_t utf8_encode
    (
        std::uint32_t,
        std::uint8_t *
    );

}