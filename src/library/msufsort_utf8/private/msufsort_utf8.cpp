#include "./msufsort_utf8.h"

#include <library/msufsort_utf8/filter/utf-8.h>

#include <include/endian.h>

#include <range/v3/view/enumerate.hpp>

#include <iostream>
#include <chrono>


//#define VERBOSE
//#define VALIDATE


struct maniscalco::msufsort::stack_frame
{
    std::span<suffix_index> range_;
    std::uint32_t           matchLength_;
};


//=============================================================================
maniscalco::msufsort::msufsort
(
    // construct msufsort instance configured with the specified number of threads.
    configuration const & config
)
{
}


//=============================================================================
auto maniscalco::msufsort::suffix_array
(
    std::span<symbol const> source
) -> std::vector<suffix_index> 
{
    #ifdef VERBOSE
        auto start = std::chrono::system_clock::now();
    #endif


    getValueEnd_ = (source.data() + source.size() - sizeof(suffix_value));

    // locate each suffix (utf-8 aware)
    std::array<std::uint32_t, 0x10000> counters{};
    auto suffixArray = select_suffixes(source, counters);
    sa_ = suffixArray.data();
    isa_ = (suffixArray.data() + ((source.size() + 1)/ 2));

    #ifdef VERBOSE
        auto finish = std::chrono::system_clock::now();
        auto elapsed = finish - start;
        std::cout << "total suffixes = " << suffixArray.size() << std::endl;
        std::cout << "elasped time for count = " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() << "ms\n";
        start = std::chrono::system_clock::now();
    #endif

    auto currentIndex = 0;
    for (auto n : counters)
    {
        if (n > currentIndex)
        {
            multikey_quicksort(source, std::span(suffixArray.data() + currentIndex, n - currentIndex), 2);
            currentIndex = n;
        }
    }

    #ifdef VERBOSE
        finish = std::chrono::system_clock::now();
        elapsed = finish - start;
        std::cout << "elasped time for quicksort = " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() << "ms                             \n";
    #endif

    #ifdef VALIDATE
        auto ptr = suffixArray.data();
        for (auto j = 1; j < currentIndex; ++j)
            if (compare_suffixes(source, source.data(), ptr[j - 1], ptr[j]) == true)
            {
                std::cout << "Sort error detected\n";
                throw 0;
            }
        std::cout << "sort validated\n";
    #endif
    return suffixArray;
}


//=============================================================================
auto maniscalco::msufsort::select_suffixes
(
    // for utf-8 input.  locate each symbol
    std::span<symbol const> source,
    std::array<std::uint32_t, 0x10000> & counters
) -> std::vector<suffix_index>
{
    // count unique 2 byte combinations at the start of each suffix
    auto endSource = source.data() + source.size();
    auto begin = reinterpret_cast<std::uint8_t const *>(source.data());
    auto end = begin + source.size();
    auto cur = begin;
    while (cur < (end - 1))
    {
        auto [_, size] = utf8_decode(cur);
        ++counters[endian_swap<host_order_type, big_endian_type>(*(std::uint16_t const *)(cur))];
        cur += size;
    }
    if (cur < end)
        counters[((std::uint32_t)*cur) << 8]++;

    // update offset to each bucket
    auto total = 1; // one because of sentinel
    for (auto & c : counters)
    {
        auto temp = c;
        c = total;
        total += temp;
    }

    // bin each suffix (4N)
    std::vector<suffix_index> suffixArray(total);
    suffixArray[0] = source.size(); // sentinel

    cur = begin;
    while (cur < (end - 1))
    {
        auto [_, size] = utf8_decode(cur);
        suffixArray[counters[endian_swap<host_order_type, big_endian_type>(*(std::uint16_t const *)(cur))]++] = std::distance(begin, cur);
        cur += size;
    }
    if (cur < end)
        suffixArray[counters[((std::uint32_t)*cur) << 8]++] = (source.size() - 1);

    return suffixArray;
}


//==============================================================================
bool maniscalco::msufsort::compare_suffixes
(
    std::span<symbol const> source,
    symbol const * inputBegin,
    suffix_index indexA,
    suffix_index indexB
) const
{
    if (indexA > indexB)
        return !compare_suffixes(source, inputBegin, indexB, indexA);
    auto inputCurrentA = inputBegin + indexA;
    auto inputCurrentB = inputBegin + indexB;
    while ((inputCurrentB <= getValueEnd_) && (*(suffix_value const *)inputCurrentB == *(suffix_value const *)inputCurrentA))
    {
        inputCurrentB += sizeof(suffix_value);
        inputCurrentA += sizeof(suffix_value);
    }
    if (inputCurrentB >= (source.data() + source.size()))
        return true;
    return (get_value(inputCurrentA, 0) >= get_value(inputCurrentB, 0));
}


//==============================================================================
void maniscalco::msufsort::multikey_insertion_sort
(
    std::span<symbol const> source,
    std::span<suffix_index> range,
    std::int32_t currentMatchLength
)
{
    std::int32_t partitionSize = range.size();
    if (partitionSize <= 2)
    {
        if ((partitionSize == 2) && (compare_suffixes(source, source.data() + currentMatchLength, range[0], range[1])))
            std::swap(range[0], range[1]);
        return;
    }
    struct partition_info
    {
        std::int32_t currentMatchLength_;
        std::int32_t size_;
    };
    partition_info stack[insertion_sort_threshold];
    stack[0] = {currentMatchLength, partitionSize};
    auto stackTop = stack + 1;

    auto partitionBegin = range.data();

    while (stackTop-- != stack)
    {
        auto currentMatchLength = stackTop->currentMatchLength_;
        auto size = stackTop->size_;

        auto sourceOffsetByMatchLength = source.data() + currentMatchLength;
        if (size <= 2)
        {
            if ((size == 2) && (compare_suffixes(source, sourceOffsetByMatchLength, partitionBegin[0], partitionBegin[1])))
                std::swap(partitionBegin[0], partitionBegin[1]);
            partitionBegin += size;
        }
        else
        {
            suffix_value value[insertion_sort_threshold];
            value[0] = get_value(sourceOffsetByMatchLength, partitionBegin[0]);
            for (std::int32_t i = 1; i < size; ++i)
            {
                auto currentIndex = partitionBegin[i];
                suffix_value currentValue = get_value(sourceOffsetByMatchLength, currentIndex);
                auto j = i;
                while ((j > 0) && (value[j - 1] > currentValue))
                {
                    value[j] = value[j - 1];
                    partitionBegin[j] = partitionBegin[j - 1];
                    --j;
                }
                value[j] = currentValue;
                partitionBegin[j] = currentIndex;
            }

            auto i = (std::int32_t)size - 1;
            auto nextMatchLength = currentMatchLength + (std::int32_t)sizeof(suffix_value);
            while (i >= 0)
            {
                std::int32_t start = i--;
                auto startValue = value[start];
                while ((i >= 0) && (value[i] == startValue))
                    --i;
                auto partitionSize = (start - i);
                *stackTop++ = partition_info{nextMatchLength, partitionSize};
            }
        }
    }
}


//=============================================================================
auto maniscalco::msufsort::multikey_quicksort
(
    std::span<symbol const> source,
    std::span<suffix_index> range,
    std::uint32_t currentMatchLength
) -> suffix_index *
{
    std::vector<stack_frame> stack;
    stack.reserve((1 << 10) * 32);
    stack.push_back(
            {
                .range_ = std::span(range.data(), range.size()), 
                .matchLength_ = currentMatchLength
            });

    while (!stack.empty())
    {
        auto & s = stack.back();
        auto suffixArrayBegin = s.range_.data();
        auto suffixArrayEnd = suffixArrayBegin + s.range_.size();
        currentMatchLength = s.matchLength_;
        stack.pop_back();

        std::uint64_t partitionSize = std::distance(suffixArrayBegin, suffixArrayEnd);
        if (partitionSize < insertion_sort_threshold)
        {
            multikey_insertion_sort(source, std::span(suffixArrayBegin, suffixArrayEnd), currentMatchLength);
            continue;
        }

        // select three pivots
        auto offsetInputBegin = source.data() + currentMatchLength;
        auto oneSixthOfPartitionSize = (partitionSize / 6);
        auto pivotCandidate1 = suffixArrayBegin + oneSixthOfPartitionSize;
        auto pivotCandidate2 = pivotCandidate1 + oneSixthOfPartitionSize;
        auto pivotCandidate3 = pivotCandidate2 + oneSixthOfPartitionSize;
        auto pivotCandidate4 = pivotCandidate3 + oneSixthOfPartitionSize;
        auto pivotCandidate5 = pivotCandidate4 + oneSixthOfPartitionSize;
        auto pivotCandidateValue1 = get_value(offsetInputBegin, *pivotCandidate1);
        auto pivotCandidateValue2 = get_value(offsetInputBegin, *pivotCandidate2);
        auto pivotCandidateValue3 = get_value(offsetInputBegin, *pivotCandidate3);
        auto pivotCandidateValue4 = get_value(offsetInputBegin, *pivotCandidate4);
        auto pivotCandidateValue5 = get_value(offsetInputBegin, *pivotCandidate5);
        if (pivotCandidateValue1 > pivotCandidateValue2)
            std::swap(*pivotCandidate1, *pivotCandidate2), std::swap(pivotCandidateValue1, pivotCandidateValue2);
        if (pivotCandidateValue4 > pivotCandidateValue5)
            std::swap(*pivotCandidate4, *pivotCandidate5), std::swap(pivotCandidateValue4, pivotCandidateValue5);
        if (pivotCandidateValue1 > pivotCandidateValue3)
            std::swap(*pivotCandidate1, *pivotCandidate3), std::swap(pivotCandidateValue1, pivotCandidateValue3);
        if (pivotCandidateValue2 > pivotCandidateValue3)
            std::swap(*pivotCandidate2, *pivotCandidate3), std::swap(pivotCandidateValue2, pivotCandidateValue3);
        if (pivotCandidateValue1 > pivotCandidateValue4)
            std::swap(*pivotCandidate1, *pivotCandidate4), std::swap(pivotCandidateValue1, pivotCandidateValue4);
        if (pivotCandidateValue3 > pivotCandidateValue4)
            std::swap(*pivotCandidate3, *pivotCandidate4), std::swap(pivotCandidateValue3, pivotCandidateValue4);
        if (pivotCandidateValue2 > pivotCandidateValue5)
            std::swap(*pivotCandidate2, *pivotCandidate5), std::swap(pivotCandidateValue2, pivotCandidateValue5);
        if (pivotCandidateValue2 > pivotCandidateValue3)
            std::swap(*pivotCandidate2, *pivotCandidate3), std::swap(pivotCandidateValue2, pivotCandidateValue3);
        if (pivotCandidateValue4 > pivotCandidateValue5)
            std::swap(*pivotCandidate4, *pivotCandidate5), std::swap(pivotCandidateValue4, pivotCandidateValue5);
        auto pivot1 = pivotCandidateValue1;
        auto pivot2 = pivotCandidateValue3;
        auto pivot3 = pivotCandidateValue5;
        // partition seven ways
        auto curSuffix = suffixArrayBegin;
        auto beginPivot1 = suffixArrayBegin;
        auto endPivot1 = suffixArrayBegin;
        auto beginPivot2 = suffixArrayBegin;
        auto endPivot2 = suffixArrayEnd - 1;
        auto beginPivot3 = endPivot2;
        auto endPivot3 = endPivot2;

        std::swap(*curSuffix++, *pivotCandidate1);
        beginPivot2 += (pivot1 != pivot2);
        endPivot1 += (pivot1 != pivot2);
        std::swap(*curSuffix++, *pivotCandidate3);
        if (pivot2 != pivot3)
        {
            std::swap(*endPivot2--, *pivotCandidate5);
            --beginPivot3;
        }

        auto currentValue = get_value(offsetInputBegin, *curSuffix);
        auto nextValue = get_value(offsetInputBegin, curSuffix[1]);
        auto nextDValue = get_value(offsetInputBegin, *endPivot2);
        while (curSuffix <= endPivot2)
        {
            if (currentValue <= pivot2)
            {
                auto temp = nextValue;
                nextValue = get_value(offsetInputBegin, curSuffix[2]);
                if (currentValue < pivot2)
                {
                    std::swap(*beginPivot2, *curSuffix);
                    if (currentValue <= pivot1)
                    {
                        if (currentValue < pivot1)
                            std::swap(*beginPivot1++, *beginPivot2);
                        std::swap(*endPivot1++, *beginPivot2); 
                    }
                    ++beginPivot2; 
                }
                ++curSuffix;
                currentValue = temp;
            }
            else
            {
                auto temp = get_value(offsetInputBegin, endPivot2[-1]);
                std::swap(*endPivot2, *curSuffix);
                if (currentValue >= pivot3)
                {
                    if (currentValue > pivot3)
                        std::swap(*endPivot2, *endPivot3--);
                    std::swap(*endPivot2, *beginPivot3--);
                }
                --endPivot2;
                currentValue = nextDValue;
                nextDValue = temp;
            }
        }

        auto nextMatchLength = (currentMatchLength + (std::uint32_t)sizeof(suffix_value));
        if (++endPivot3 != suffixArrayEnd)
        {
            stack.push_back(
                    {
                        .range_ = std::span(endPivot3, suffixArrayEnd - endPivot3), 
                        .matchLength_ = currentMatchLength
                    });
        }
        if (++beginPivot3 != endPivot3)
        {
            stack.push_back(
                    {
                        .range_ = std::span(beginPivot3, endPivot3 - beginPivot3), 
                        .matchLength_ = nextMatchLength
                    });
        }
        if (++endPivot2 != beginPivot3)
        {
            stack.push_back(
                    {
                        .range_ = std::span(endPivot2, beginPivot3 - endPivot2), 
                        .matchLength_ = currentMatchLength
                    });
        }
        if (beginPivot2 != endPivot2)
        {
            stack.push_back(
                    {
                        .range_ = std::span(beginPivot2, endPivot2 - beginPivot2), 
                        .matchLength_ = nextMatchLength
                    });
        }
        if (endPivot1 != beginPivot2)
        {
            stack.push_back(
                    {
                        .range_ = std::span(endPivot1, beginPivot2 - endPivot1), 
                        .matchLength_ = currentMatchLength
                    });
        }
        if (beginPivot1 != endPivot1)
        {
            stack.push_back(
                    {
                        .range_ = std::span(beginPivot1, endPivot1 - beginPivot1), 
                        .matchLength_ = nextMatchLength
                    });
        }
        if (suffixArrayBegin != beginPivot1)
        {
            stack.push_back(
                    {
                        .range_ = std::span(suffixArrayBegin, beginPivot1 - suffixArrayBegin), 
                        .matchLength_ = currentMatchLength
                    });
        }
    }
    return nullptr;
}


//==============================================================================
inline auto maniscalco::msufsort::get_value
(
    symbol const * source,
    suffix_index index
) const -> suffix_value
{
    source += index;//& sa_index_mask);
    if (source <= getValueEnd_) [[likely]]
        return endian_swap<host_order_type, big_endian_type>(*(suffix_value const *)(source));
    else [[unlikely]]
        return get_value_over(source, index);
}


//=============================================================================
auto __attribute__ ((noinline)) maniscalco::msufsort::get_value_over
(
    symbol const * source,
    suffix_index index
) const -> suffix_value
{
    auto over = std::distance(getValueEnd_, source);
    return (over >= sizeof(suffix_value)) ? 0 : (endian_swap<host_order_type, big_endian_type>(*(suffix_value const *)(getValueEnd_)) << (over * 8));
}
