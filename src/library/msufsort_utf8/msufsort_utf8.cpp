#include "./msufsort_utf8.h"

#include "./private/msufsort_utf8.h"
#include "./filter/utf-8.h"
#include <include/endian.h>

#include <range/v3/view/enumerate.hpp>

#include <fstream>
#include <map>
#include <iostream>
#include <unordered_set>


//=============================================================================
auto maniscalco::make_suffix_array
(
    std::span<std::byte const> source
) -> std::vector<std::uint32_t>
{
    return msufsort({}).suffix_array(source);
}


void reorder
(
    std::span<std::uint32_t> data
)
{
    std::vector<std::uint32_t> count(1 << 21);

    std::uint32_t step = 1;
    for (auto cur = data.begin(); cur < data.end(); cur += step)
    {
        count[*cur]++;
        step += 3;
        step &= 15;
    }

    std::vector<std::pair<std::uint32_t, std::uint32_t>> order;
    for (auto && [index, c] : ranges::view::enumerate(count))
        if (c > 0)
            order.push_back({c, index});
    std::sort(order.begin(), order.end(), [](auto a, auto b){return (a > b);});

    auto n = 1ull;
    for (auto [_, index] : order)
        count[index] = n++;

    for (auto & value : data)
    {
        auto rank = count[value];
        if (rank == 0)
            rank = count[value] = n++;
        value = rank - 1;
    }
}


//=============================================================================
std::vector<std::byte> maniscalco::make_burrows_wheeler_transform
(
    std::span<std::byte const> source
)
{
    auto start = std::chrono::system_clock::now();

    // do utf-8 aware suffix array
    auto suffixArray = make_suffix_array(source);

    // do BWT
    std::vector<std::byte> output((suffixArray.size() - 1) * 4); // -1 for sentinel
    auto beginOut = reinterpret_cast<std::uint32_t *>(output.data());
    auto out = beginOut;
    auto endOut = beginOut + suffixArray.size() - 1;
    auto sentinelIndex = 0;

    for (auto && [index, suffixIndex] : ranges::views::enumerate(suffixArray))
    {
        if (suffixIndex == 0)
        {
            sentinelIndex = index;
            continue;
        }
        auto end = (reinterpret_cast<std::uint8_t const *>(source.data()) + suffixIndex);
        auto cur = end - 1;
        while ((*cur & 0xc0) == 0x80)
            --cur;
        auto [value, _] = utf8_decode(reinterpret_cast<std::uint8_t const *>(cur));
        *out++ = value;
    }
    auto finish = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
    std::cout << "Elapsed forward BWT time = " << ((double)elapsed / 1000) << " sec\n";

    // validate by inverting and comparing
    // test 
    auto decoded = reverse_burrows_wheeler_transform(output, source.size(), sentinelIndex);
    bool success = true;
    for (auto i = 0; i < source.size(); ++i)
        if (source[i] != decoded[i])
            success = false;
    if (!success)
        std::cout << "FAIELD to reverse BWT\n";
    else
        std::cout << "BWT validated\n";

    return output;
}


//=============================================================================
std::vector<std::byte> maniscalco::reverse_burrows_wheeler_transform
(
    std::span<std::byte const> input,
    std::uint32_t outputSize,
    std::uint32_t sentinelIndex
)
{
    auto start = std::chrono::system_clock::now();

    std::span<std::uint32_t const> source(reinterpret_cast<std::uint32_t const *>(input.data()), (input.size() / 4) + 1);
    // this bit won't be needed eventually because the m99 decoder will provide the counts
    std::unordered_map<std::uint32_t, std::uint32_t> counter;
    auto beginSource = source.data();
    auto endSource = beginSource + source.size() - 1;
    auto uniqueCount = 0;

    for (auto curSource = beginSource; curSource < endSource; ++curSource)
        counter[*curSource]++;

    std::vector<std::pair<std::uint32_t, std::uint32_t>> sortedCounter(counter.size());
    auto n = 0;
    for (auto iter : counter)
        sortedCounter[n++] = {iter.first, iter.second};
    std::sort(sortedCounter.begin(), sortedCounter.end());

    // calculate ranks
    auto total = 1;
    for (auto [value, count] : sortedCounter)
    {
        counter[value] = total;
        total += count;
    }

    // (4N)
    std::vector<std::uint32_t> reverseMap(source.size());
    auto curSource = beginSource;
    for (auto index = 0; index < source.size(); ++index)
        reverseMap[counter[*(curSource++)]++] = (index + (index >= sentinelIndex));
    reverseMap[0] = sentinelIndex;

    // (1N)
    std::vector<std::byte> decoded(outputSize);
    auto curDec = reinterpret_cast<std::uint8_t *>(decoded.data());
    auto endDec = curDec + decoded.size();
    auto index = sentinelIndex;
    while (curDec < endDec)
    {
        index = reverseMap[index];
        curDec += utf8_encode(beginSource[index - (index >= sentinelIndex)], curDec);
    }

    auto finish = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
    std::cout << "Elapsed reverse BWT time = " << ((double)elapsed / 1000) << " sec\n";

    return decoded;
}
