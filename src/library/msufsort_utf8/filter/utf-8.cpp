#include "./utf-8.h"

#include <fstream>
#include <iostream>
#include <map>

#define VALIDATE_UTF
#define VERBOSE_UTF


//=============================================================================
std::pair<std::uint32_t, std::uint8_t> maniscalco::utf8_decode
(
    std::uint8_t const * source
)
{
    std::uint32_t n = *source;
    if ((n & 0x80) == 0x00)
    {
        // one byte
        return {n, 1};
    }
    if ((n & 0xe0) == 0xc0)
    {
        // two byte
        return {((n & 0x1f) << 6) | (source[1] & 0x3f), 2};
    }
    if ((n & 0xf0) == 0xe0)
    {
        // three byte
        return {((n & 0x0f) << 12) | (((source[1] & 0x3f)) << 6) | ((source[2] & 0x3f)), 3};
    }     
    if ((n & 0xf8) == 0xf0)
    {
        // 4 byte
        return {((n & 0x07) << 18) | (((source[1] & 0x3f)) << 12) | 
                                ((((source[2] & 0x3f)) << 6))| ((source[3] & 0x3f)), 4};
    }
    throw 0;
}


//=============================================================================
std::uint8_t maniscalco::utf8_encode
(
    std::uint32_t value,
    std::uint8_t * dest
)
{
    if (value < (1ull << 7))
    {
        // 1 byte
        *dest = value;
        return 1;
    }
    if (value <= (1ull << 11))
    {
        // 2 byte
        *dest++ = (0xc0 | ((value >> 6) & 0x1f));
        *dest = (0x80 | (value & 0x3f));
        return 2;
    }
    if (value <= (1ull << 16))
    {
        // 3 byte
        *dest++ = (0xe0 | ((value >> 12) & 0x0f));
        *dest++ = (0x80 | ((value >> 6) & 0x3f)); 
        *dest = (0x80 | (value & 0x3f));     
        return 3;     
    }
    // 4 byte
    *dest++ = (0xf0 | ((value >> 18) & 0x03));
    *dest++ = (0x80 | ((value >> 12) & 0x3f)); 
    *dest++ = (0x80 | ((value >> 6) & 0x3f)); 
    *dest++ = (0x80 | (value & 0x3f)); 
    return 4;    
}


//=============================================================================
std::vector<std::uint32_t> maniscalco::utf8_filter
(
    std::span<std::uint8_t const> input
)
{
    // assuming no utf encode errors, calculate the number of unqiue symbols in the input
    auto symbolCount{0};
    for (auto c : input)
        symbolCount += ((c & 0xc0) != 0x80);
    std::vector<std::uint32_t> decoded;
    decoded.resize(symbolCount);

    // decode the input into 32 bit integers
    auto beginDec = decoded.data();
    auto curDec = beginDec;

    auto begin = input.data();
    auto end = begin + input.size();
    for (auto cur = begin; cur < end; )
    {
        auto [value, size] = utf8_decode(cur);
        cur += size;
        *curDec++ = value;
    }
    decoded.resize(std::distance(beginDec, curDec));

    #ifdef VALIDATE_UTF
        std::vector<std::uint8_t> reproduced;
        reproduced.resize(input.size());
        auto dest = reproduced.data();
        for (auto value : decoded)
            dest += utf8_encode(value, dest);

        bool success = true;
        for (auto i = 0; i < input.size(); ++i)
            if (input[i] != reproduced[i])
                success = false;
        if (success)
            std::cout << "filter validated\n";
        else
            std::cout << "filter ERROR\n";
    #endif

    #ifdef VERBOSE_UTF
        std::map<std::uint32_t, std::uint32_t> count;
        for (auto c : decoded)
            count[c]++;
        std::cout << "unique symbol count = " << count.size() << std::endl;
    #endif
    
    return decoded;
}