#pragma once

#include "util/radix_sort.hpp"
#include "debug/assert.hpp"
#include <list>

template<typename T>
struct SkaSorter;

template<typename T>
struct SkaSortContainerAccess;
template<typename T>
struct SkaSortContainerCompare;

template<typename It>
void ska_sort2(It begin, It end);
template<typename It, typename ExtractKey>
void ska_sort2(It begin, It end, ExtractKey && extract_key);

namespace detail
{

template<typename SortSettings, bool Backwards, typename It, typename FallbackCompare, typename CurrentExtractKey>
class SkaSortImpl
{
    template<typename, bool, typename, typename, typename>
    friend class SkaSortImpl;

    It stored_begin;
    It stored_end;
    FallbackCompare fallback_compare;
    CurrentExtractKey current_extract_key;
    void (*next_sort_up)(It begin, It end, const void * up_sort_impl) = nullptr;
    const void * up_sort_impl = nullptr;

    using SortFunctionType = void (*)(SkaSortImpl &);
    SortFunctionType sort_next_from_callback = nullptr;
    void * sort_next_from_callback_function = nullptr;
    void * sort_next_from_callback_data = nullptr;

    template<typename ExtractByte>
    void byte_sort(It begin, std::ptrdiff_t num_elements, ExtractByte & extract_byte)
    {
        if (num_elements < SortSettings::AmericanFlagSortUpperLimit)
        {
            if (num_elements <= std::numeric_limits<uint8_t>::max())
                american_flag_sort<uint8_t>(begin, num_elements, extract_byte);
            else
                american_flag_sort<typename SortSettings::count_type>(begin, num_elements, extract_byte);
        }
        else
            ska_byte_sort<typename SortSettings::count_type>(begin, num_elements, extract_byte);
    }

    void __attribute__((always_inline)) recurse_after_sort(It begin, It end, std::ptrdiff_t num_elements)
    {
        if constexpr (Backwards)
        {
            if (DefaultSortIfLessThanThreshold<SortSettings>(begin, end, num_elements, [fallback_compare = fallback_compare](auto && l, auto && r){ return fallback_compare(r, l); }))
                return;
        }
        else
        {
            if (DefaultSortIfLessThanThreshold<SortSettings>(begin, end, num_elements, fallback_compare))
                return;
        }

        if (sort_next_from_callback)
        {
            this->stored_begin = begin;
            this->stored_end = end;
            sort_next_from_callback(*this);
        }
        else
        {
            next_sort_up(begin, end, up_sort_impl);
        }
    }

    template<typename count_type, typename ExtractByte>
    void __attribute__((noinline)) american_flag_sort(It begin, std::ptrdiff_t num_elements, ExtractByte extract_byte)
    {
        count_type counts_and_offsets[256] = {};
        FixedSizeBitSet<256> partitions_used;
        count_type partition_ends[256];
        unroll_loop_nonempty<SortSettings::FirstLoopUnrollAmount>(begin, num_elements, [extract_byte, counts_and_offsets = &*counts_and_offsets, &partitions_used](It it)
        {
            uint8_t index = extract_byte(*it);
            ++counts_and_offsets[index];
            partitions_used.SetBit(index);
        });
        count_type total = 0;
        uint8_t remaining_partitions[256];
        int num_partitions = 0;
        partitions_used.ForEachSetBit([&](int i)
        {
            count_type count = std::exchange(counts_and_offsets[i], total);
            total += count;
            partition_ends[i] = total;
            remaining_partitions[num_partitions] = i;
            ++num_partitions;
        });
        if (num_partitions > 1)
        {
            uint8_t * current_block_ptr = remaining_partitions;
            uint8_t current_block = *current_block_ptr;
            uint8_t * second_to_last_block = remaining_partitions + num_partitions - 2;
            It it = begin;
            It block_end = begin + partition_ends[current_block];
            for (;;)
            {
                for (;;)
                {
                    uint8_t destination = extract_byte(*it);
                    count_type destination_index = counts_and_offsets[destination]++;
                    if (destination == current_block)
                        break;
                    auto tmp = std::move(begin[destination_index]);
                    destination = extract_byte(tmp);
                    begin[destination_index] = std::move(*it);
                    destination_index = counts_and_offsets[destination]++;
                    if (destination == current_block)
                    {
                        begin[destination_index] = std::move(tmp);
                        break;
                    }
                    *it = std::move(begin[destination_index]);
                    begin[destination_index] = std::move(tmp);
                }
                ++it;
                if (it != block_end)
                    continue;
                for (;;)
                {
                    if (current_block_ptr == second_to_last_block)
                        goto recurse;
                    ++current_block_ptr;
                    current_block = *current_block_ptr;
                    count_type current_block_start = counts_and_offsets[current_block];
                    count_type current_block_end = partition_ends[*current_block_ptr];
                    if (current_block_start != current_block_end)
                    {
                        it = begin + current_block_start;
                        block_end = begin + current_block_end;
                        break;
                    }
                }
            }
        }
        recurse:
        if (!sort_next_from_callback && !next_sort_up)
            return;

        count_type start_offset = 0;
        It partition_begin = begin;
        for (uint8_t * it = remaining_partitions, * remaining_end = remaining_partitions + num_partitions;;)
        {
            count_type end_offset = partition_ends[*it];
            It partition_end = begin + end_offset;
            recurse_after_sort(partition_begin, partition_end, end_offset - start_offset);
            start_offset = end_offset;
            partition_begin = partition_end;
            ++it;
            if (it == remaining_end)
                break;
        }
    }

    template<typename count_type, typename ExtractByte>
    void __attribute__((noinline)) ska_byte_sort(It begin, std::ptrdiff_t num_elements, ExtractByte extract_byte)
    {
        count_type counts_and_offsets[256] = {};
        FixedSizeBitSet<256> partitions_used;
        count_type partition_ends_plus_one[257];
        partition_ends_plus_one[0] = 0;
        count_type * const partition_ends = partition_ends_plus_one + 1;
        unroll_loop_nonempty<SortSettings::FirstLoopUnrollAmount>(begin, num_elements, [extract_byte, counts_and_offsets = &*counts_and_offsets, &partitions_used](It it)
        {
            uint8_t index = extract_byte(*it);
            ++counts_and_offsets[index];
            partitions_used.SetBit(index);
        });
        uint8_t remaining_partitions[256];
        count_type total = 0;
        int num_partitions = 0;
        partitions_used.ForEachSetBit([&](int i)
        {
            count_type * count_and_offset = counts_and_offsets + i;
            count_type * partition_end = partition_ends + i;
            count_type count = *count_and_offset;
            *count_and_offset = total;
            partition_end[-1] = total;
            total += count;
            *partition_end = total;
            remaining_partitions[num_partitions] = i;
            ++num_partitions;
        });
        for (uint8_t * last_remaining = remaining_partitions + num_partitions, * end_partition = remaining_partitions + 1; last_remaining > end_partition;)
        {
            last_remaining = custom_std_partition(remaining_partitions, last_remaining, [&](uint8_t partition)
            {
                count_type & begin_offset = counts_and_offsets[partition];
                count_type end_offset = partition_ends[partition];
                if (begin_offset == end_offset)
                    return false;

                if constexpr (SortSettings::ThreeWaySwapBoost)
                {
                    unroll_loop_nonempty<SortSettings::SecondLoopUnrollAmount>(begin + begin_offset, end_offset - begin_offset, [begin, partition, counts_and_offsets = &*counts_and_offsets, extract_byte](It it)
                    {
                        uint8_t target_partition = extract_byte(*it);
                        count_type offset = counts_and_offsets[target_partition]++;
                        It target_it = begin + offset;
                        uint8_t target_of_target_partition = extract_byte(*target_it);
                        auto tmp = std::move(*it);
                        if (target_of_target_partition == partition)
                        {
                            *it = std::move(*target_it);
                        }
                        else
                        {
                            It target_of_target_it = begin + counts_and_offsets[target_of_target_partition]++;
                            *it = std::move(*target_of_target_it);
                            *target_of_target_it = std::move(*target_it);
                        }
                        *target_it = std::move(tmp);
                    });
                }
                else if constexpr (SortSettings::ThreeWaySwapStepanov)
                {
                    unroll_loop_nonempty<SortSettings::SecondLoopUnrollAmount>(begin + begin_offset, end_offset - begin_offset, [begin, counts_and_offsets = &*counts_and_offsets, extract_byte](It it)
                    {
                        uint8_t destination_partition = extract_byte(*it);
                        It destination = begin + counts_and_offsets[destination_partition]++;
                        if (destination == it)
                            return;
                        auto tmp = std::move(*destination);
                        destination_partition = extract_byte(tmp);
                        *destination = std::move(*it);
                        destination = begin + counts_and_offsets[destination_partition]++;
                        *it = std::move(*destination);
                        *destination = std::move(tmp);
                    });
                }
                else
                {
                    unroll_loop_nonempty<SortSettings::SecondLoopUnrollAmount>(begin + begin_offset, end_offset - begin_offset, [begin, counts_and_offsets = &*counts_and_offsets, extract_byte](It it)
                    {
                        uint8_t target_partition = extract_byte(*it);
                        count_type offset = counts_and_offsets[target_partition]++;
                        std::iter_swap(it, begin + offset);
                    });
                }
                return begin_offset != end_offset;
            });
        }
        if (!sort_next_from_callback && !next_sort_up)
            return;

        for (uint8_t * it = remaining_partitions + num_partitions;;)
        {
            --it;
            uint8_t partition = *it;
            count_type start_offset = partition_ends[partition - 1];
            count_type end_offset = partition_ends[partition];
            recurse_after_sort(begin + start_offset, begin + end_offset, end_offset - start_offset);
            if (it == remaining_partitions)
                break;
        }
    }

    template<typename ExtractBool>
    void __attribute__((noinline)) bool_sort(It begin, It end, ExtractBool && extract_bool)
    {
        It middle = std::partition(begin, end, [&](auto && a)
        {
            return !extract_bool(a);
        });
        if (!sort_next_from_callback && !next_sort_up)
            return;
        recurse_after_sort(begin, middle, middle - begin);
        recurse_after_sort(middle, end, end - middle);
    }

    static void sort_from_recurse(It begin, It end, const void * void_self)
    {
        const SkaSortImpl * self = static_cast<const SkaSortImpl *>(void_self);
        SkaSortImpl recurse(*self);
        recurse.stored_begin = begin;
        recurse.stored_end = end;
        recurse.sort_next_from_callback(recurse);
    }
    static void sort_from_recurse_reverse(std::reverse_iterator<It> begin, std::reverse_iterator<It> end, const void * void_self)
    {
        const SkaSortImpl * self = static_cast<const SkaSortImpl *>(void_self);
        SkaSortImpl recurse(*self);
        recurse.stored_begin = end.base();
        recurse.stored_end = begin.base();
        recurse.sort_next_from_callback(recurse);
    }
    static void sort_up_reverse(std::reverse_iterator<It> begin, std::reverse_iterator<It> end, const void * void_self)
    {
        const SkaSortImpl * self = static_cast<const SkaSortImpl *>(void_self);
        SkaSortImpl recurse(*self);
        recurse.next_sort_up(end.base(), begin.base(), recurse.up_sort_impl);
    }
    template<typename Argument>
    static void SortNextFromCallback(SkaSortImpl & self)
    {
        void (*next_sort)(SkaSortImpl &, Argument) = reinterpret_cast<void (*)(SkaSortImpl &, Argument)>(self.sort_next_from_callback_function);
        next_sort(self, *static_cast<Argument *>(self.sort_next_from_callback_data));
    }

    template<typename NextKey>
    void sort_with_key(NextKey && next_key)
    {
        auto next_key_combined = [next_key, current_extract_key = current_extract_key](auto && item) -> decltype(auto)
        {
            return next_key(current_extract_key(item));
        };
        using NextKeyType = decltype(next_key_combined(*stored_begin));
        if constexpr (std::is_same_v<unsigned char, NextKeyType>)
        {
            byte_sort(stored_begin, stored_end - stored_begin, next_key_combined);
        }
        else if constexpr (std::is_same_v<bool, NextKeyType>)
        {
            bool_sort(stored_begin, stored_end, next_key_combined);
        }
        else
        {
            SkaSortImpl<SortSettings, Backwards, It, FallbackCompare, decltype(next_key_combined)> recurse_sorter{stored_begin, stored_end, fallback_compare, next_key_combined};
            if (sort_next_from_callback)
            {
                recurse_sorter.next_sort_up = &sort_from_recurse;
                recurse_sorter.up_sort_impl = this;
            }
            else if (next_sort_up)
            {
                recurse_sorter.next_sort_up = next_sort_up;
                recurse_sorter.up_sort_impl = up_sort_impl;
            }
            SkaSorter<NextKeyType>()(recurse_sorter);
        }
    }

    template<typename NextKey>
    void sort_with_key_reverse(NextKey && next_key)
    {
        auto next_key_combined = [next_key, current_extract_key = current_extract_key](auto && item) -> decltype(auto)
        {
            return next_key(current_extract_key(item));
        };
        std::reverse_iterator<It> reverse_begin(stored_end);
        std::reverse_iterator<It> reverse_end(stored_begin);
        using NextKeyType = decltype(next_key_combined(*stored_begin));
        SkaSortImpl<SortSettings, !Backwards, std::reverse_iterator<It>, FallbackCompare, decltype(next_key_combined)> recurse_sorter{reverse_begin, reverse_end, fallback_compare, next_key_combined};
        if (sort_next_from_callback)
        {
            recurse_sorter.next_sort_up = &sort_from_recurse_reverse;
            recurse_sorter.up_sort_impl = this;
        }
        else if (next_sort_up)
        {
            recurse_sorter.next_sort_up = &sort_up_reverse;
            recurse_sorter.up_sort_impl = this;
        }
        SkaSorter<NextKeyType>()(recurse_sorter);
    }


public:

    SkaSortImpl(It begin, It end, FallbackCompare fallback_compare, CurrentExtractKey current_extract_key)
        : stored_begin(begin), stored_end(end), fallback_compare(std::move(fallback_compare)), current_extract_key(std::move(current_extract_key))
    {
    }

    using SortSettingsType = SortSettings;

    template<typename NextKey, typename NextSortArg>
    void sort(NextKey && next_key, void (*next_sort)(SkaSortImpl &, NextSortArg), NextSortArg next_sort_arg)
    {
        void (*old_next)(SkaSortImpl &) = std::exchange(sort_next_from_callback, &SortNextFromCallback<NextSortArg>);
        void * old_function = std::exchange(sort_next_from_callback_function, reinterpret_cast<void *>(next_sort));
        void * old_data = std::exchange(sort_next_from_callback_data, std::addressof(next_sort_arg));
        sort_with_key(std::forward<NextKey>(next_key));
        sort_next_from_callback = old_next;
        sort_next_from_callback_function = old_function;
        sort_next_from_callback_data = old_data;
    }
    template<typename NextKey>
    void sort(NextKey && next_key, void (*next_sort)(SkaSortImpl &) = nullptr)
    {
        void (*old_next)(SkaSortImpl &) = std::exchange(sort_next_from_callback, next_sort);
        sort_with_key(std::forward<NextKey>(next_key));
        sort_next_from_callback = old_next;
    }
    template<typename NextKey, typename Compare, typename NextSortArg>
    void sort_with_faster_comparison(NextKey && next_key, Compare comparison, void (*next_sort)(SkaSortImpl &, NextSortArg), NextSortArg next_sort_arg)
    {
        if (!SortSettings::UseFasterCompare || next_sort_up)
        {
            // the current_extract_key isn't the last item in the chain
            // for example we might be sorting a std::pair<std::string, bool>
            // and we are hitting the fallback on the string case. in this
            // case we can't use the custom comparison, because that wouldn't
            // take into account the bool. so we have to use the normal fallback
            sort(std::forward<NextKey>(next_key), next_sort, next_sort_arg);
        }
        else
        {
            auto faster_compare = [comparison, current_extract_key = current_extract_key](auto && l, auto && r)
            {
                return comparison(current_extract_key(l), current_extract_key(r));
            };
            void (*old_next)(SkaSortImpl &) = std::exchange(sort_next_from_callback, &SortNextFromCallback<NextSortArg>);
            void * old_function = std::exchange(sort_next_from_callback_function, reinterpret_cast<void *>(next_sort));
            void * old_data = std::exchange(sort_next_from_callback_data, std::addressof(next_sort_arg));
            SkaSortImpl<SortSettings, Backwards, It, decltype(faster_compare), CurrentExtractKey> custom_compare_sorter{stored_begin, stored_end, std::move(faster_compare), current_extract_key};
            custom_compare_sorter.next_sort_up = &sort_from_recurse;
            custom_compare_sorter.up_sort_impl = this;
            custom_compare_sorter.sort(std::forward<NextKey>(next_key));
            sort_next_from_callback = old_next;
            sort_next_from_callback_function = old_function;
            sort_next_from_callback_data = old_data;
        }
    }
    template<typename NextKey, typename Compare>
    void sort_with_faster_comparison(NextKey && next_key, Compare comparison, void (*next_sort)(SkaSortImpl &) = nullptr)
    {
        if (!SortSettings::UseFasterCompare || next_sort_up)
        {
            // the current_extract_key isn't the last item in the chain
            // for example we might be sorting a std::pair<std::string, bool>
            // and we are hitting the fallback on the string case. in this
            // case we can't use the custom comparison, because that wouldn't
            // take into account the bool. so we have to use the normal fallback
            sort(std::forward<NextKey>(next_key), next_sort);
        }
        else
        {
            auto faster_compare = [comparison, current_extract_key = current_extract_key](auto && l, auto && r)
            {
                return comparison(current_extract_key(l), current_extract_key(r));
            };
            void (*old_next)(SkaSortImpl &) = std::exchange(sort_next_from_callback, next_sort);
            SkaSortImpl<SortSettings, Backwards, It, decltype(faster_compare), CurrentExtractKey> custom_compare_sorter{stored_begin, stored_end, std::move(faster_compare), current_extract_key};
            if (next_sort)
            {
                custom_compare_sorter.next_sort_up = &sort_from_recurse;
                custom_compare_sorter.up_sort_impl = this;
            }
            custom_compare_sorter.sort(std::forward<NextKey>(next_key));
            sort_next_from_callback = old_next;
        }
    }

    template<typename NextKey, typename NextSortArg>
    void sort_backwards(NextKey && next_key, void (*next_sort)(SkaSortImpl &, NextSortArg), NextSortArg next_sort_arg)
    {
        void (*old_next)(SkaSortImpl &) = std::exchange(sort_next_from_callback, &SortNextFromCallback<NextSortArg>);
        void * old_function = std::exchange(sort_next_from_callback_function, reinterpret_cast<void *>(next_sort));
        void * old_data = std::exchange(sort_next_from_callback_data, std::addressof(next_sort_arg));
        sort_with_key_reverse(std::forward<NextKey>(next_key));
        sort_next_from_callback = old_next;
        sort_next_from_callback_function = old_function;
        sort_next_from_callback_data = old_data;
    }
    template<typename NextKey>
    void sort_backwards(NextKey && next_key, void (*next_sort)(SkaSortImpl &) = nullptr)
    {
        void (*old_next)(SkaSortImpl &) = std::exchange(sort_next_from_callback, next_sort);
        sort_with_key_reverse(std::forward<NextKey>(next_key));
        sort_next_from_callback = old_next;
    }
    template<typename NextKey, typename Compare, typename NextSortArg>
    void sort_backwards_with_faster_comparison(NextKey && next_key, Compare comparison, void (*next_sort)(SkaSortImpl &, NextSortArg), NextSortArg next_sort_arg)
    {
        if (!SortSettings::UseFasterCompare || next_sort_up)
        {
            // the current_extract_key isn't the last item in the chain
            // for example we might be sorting a std::pair<std::string, bool>
            // and we are hitting the fallback on the string case. in this
            // case we can't use the custom comparison, because that wouldn't
            // take into account the bool. so we have to use the normal fallback
            sort_backwards(std::forward<NextKey>(next_key), next_sort, next_sort_arg);
        }
        else
        {
            auto faster_compare = [comparison, current_extract_key = current_extract_key](auto && l, auto && r)
            {
                return comparison(current_extract_key(l), current_extract_key(r));
            };
            void (*old_next)(SkaSortImpl &) = std::exchange(sort_next_from_callback, &SortNextFromCallback<NextSortArg>);
            void * old_function = std::exchange(sort_next_from_callback_function, reinterpret_cast<void *>(next_sort));
            void * old_data = std::exchange(sort_next_from_callback_data, std::addressof(next_sort_arg));
            SkaSortImpl<SortSettings, Backwards, It, decltype(faster_compare), CurrentExtractKey> custom_compare_sorter{stored_begin, stored_end, std::move(faster_compare), current_extract_key};
            custom_compare_sorter.next_sort_up = &sort_from_recurse;
            custom_compare_sorter.up_sort_impl = this;
            custom_compare_sorter.sort_backwards(std::forward<NextKey>(next_key));
            sort_next_from_callback = old_next;
            sort_next_from_callback_function = old_function;
            sort_next_from_callback_data = old_data;
        }
    }
    template<typename NextKey, typename Compare>
    void sort_backwards_with_faster_comparison(NextKey && next_key, Compare comparison, void (*next_sort)(SkaSortImpl &) = nullptr)
    {
        if (!SortSettings::UseFasterCompare || next_sort_up)
        {
            // the current_extract_key isn't the last item in the chain
            // for example we might be sorting a std::pair<std::string, bool>
            // and we are hitting the fallback on the string case. in this
            // case we can't use the custom comparison, because that wouldn't
            // take into account the bool. so we have to use the normal fallback
            sort_backwards(std::forward<NextKey>(next_key), next_sort);
        }
        else
        {
            auto faster_compare = [comparison, current_extract_key = current_extract_key](auto && l, auto && r)
            {
                return comparison(current_extract_key(l), current_extract_key(r));
            };
            void (*old_next)(SkaSortImpl &) = std::exchange(sort_next_from_callback, next_sort);
            SkaSortImpl<SortSettings, Backwards, It, decltype(faster_compare), CurrentExtractKey> custom_compare_sorter{stored_begin, stored_end, std::move(faster_compare), current_extract_key};
            if (next_sort)
            {
                custom_compare_sorter.next_sort_up = &sort_from_recurse;
                custom_compare_sorter.up_sort_impl = this;
            }
            custom_compare_sorter.sort_backwards(std::forward<NextKey>(next_key));
            sort_next_from_callback = old_next;
        }
    }

    decltype(auto) first_item() const
    {
        return current_extract_key(*stored_begin);
    }
    template<typename Callback>
    void for_each_item(Callback && callback) const
    {
        for (It it = stored_begin, end = stored_end; it != end; ++it)
        {
            if (!callback(current_extract_key(*it)))
                break;
        }
    }
    size_t num_items() const
    {
        return stored_end - stored_begin;
    }

    void skip()
    {
        if (next_sort_up)
            next_sort_up(stored_begin, stored_end, up_sort_impl);
    }
    void std_sort_fallback()
    {
        std::sort(stored_begin, stored_end, fallback_compare);
    }
    template<typename CustomCompare>
    void std_sort_fallback(CustomCompare && compare)
    {
        if (next_sort_up)
        {
            // the current_extract_key isn't the last item in the chain
            // for example we might be sorting a std::pair<std::string, bool>
            // and we are hitting the fallback on the string case. in this
            // case we can't use the custom comparison, because that wouldn't
            // take into account the bool. so we have to use the normal fallback
            std_sort_fallback();
        }
        else
        {
            std::sort(stored_begin, stored_end, [compare, current_extract_key = current_extract_key](auto && l, auto && r)
            {
                if constexpr (Backwards)
                {
                    return compare(current_extract_key(r), current_extract_key(l));
                }
                else
                {
                    return compare(current_extract_key(l), current_extract_key(r));
                }
            });
        }
    }

    template<typename NewIt, typename ExtractKey, typename Compare>
    auto change_list_to_sort(NewIt begin, NewIt end, ExtractKey extract_key, Compare compare) const
    {
        return SkaSortImpl<SortSettings, false, NewIt, Compare, ExtractKey>{begin, end, compare, extract_key};
    }
};

struct ByValueToUnsignedSorter
{
    template<typename Sorter>
    void operator()(Sorter & sorter) const
    {
        sorter.sort([](auto value)
        {
            return to_unsigned_or_bool(value);
        });
    }
};
template<size_t Size>
struct SizedUnsignedSorter
{
    template<size_t Index, typename Sorter>
    static void SortByte(Sorter & sorter)
    {
        if constexpr (Index == 0)
        {
            sorter.sort([](auto value)
            {
                return static_cast<unsigned char>(value & 0xff);
            });
        }
        else
        {
            sorter.sort([](auto value)
            {
                static constexpr size_t ToShift = Index * 8;
                static constexpr decltype(value) mask = static_cast<decltype(value)>(0xff) << ToShift;
                return static_cast<unsigned char>((value & mask) >> ToShift);
            }, &SortByte<Index - 1>);
        }
    }

    template<typename Sorter>
    void operator()(Sorter & sorter) const
    {
        SortByte<Size - 1>(sorter);
    }
};

template<typename T, size_t Size = sizeof(T)>
struct FloatSorter
{
    static_assert(std::numeric_limits<T>::is_iec559);

    struct CompareAsUint
    {
        static uint16_t as_uint(float f)
        {
            union
            {
                float as_float;
                uint32_t as_uint;
            } in_union = { f };
            return in_union.as_uint & 0x0000'ffff;
        }
        static uint64_t as_uint(double d)
        {
            union
            {
                double as_float;
                uint64_t as_uint;
            } in_union = { d };
            return in_union.as_uint & 0x0000'ffff'ffff'ffff;
        }
        bool operator()(float l, float r) const
        {
            return as_uint(l) < as_uint(r);
        }
        bool operator()(double l, double r) const
        {
            return as_uint(l) < as_uint(r);
        }
        bool operator()(long double l, long double r) const
        {
            return l < r;
        }
    };
    struct CompareAsUintNegative : CompareAsUint
    {
        template<typename F>
        bool operator()(F l, F r) const
        {
            return static_cast<const CompareAsUint &>(*this)(r, l);
        }
    };

    template<size_t Index, typename Sorter>
    static void SortByte(Sorter & sorter)
    {
        if constexpr (Index == 0)
        {
            sorter.sort_with_faster_comparison([](T value)
            {
                return GetByte<Index>(value);
            }, CompareAsUint{});
        }
        else
        {
            sorter.sort_with_faster_comparison([](T value)
            {
                return GetByte<Index>(value);
            }, CompareAsUint{}, &SortByte<Index - 1>);
        }
    }
    template<size_t Index, typename Sorter>
    static void SortByteBackwards(Sorter & sorter)
    {
        if constexpr (Index == 0)
        {
            sorter.sort_backwards_with_faster_comparison([](T value)
            {
                return GetByte<Index>(value);
            }, CompareAsUintNegative{});
        }
        else
        {
            sorter.sort_backwards_with_faster_comparison([](T value)
            {
                return GetByte<Index>(value);
            }, CompareAsUintNegative{}, &SortByteBackwards<Index - 1>);
        }
    }
    template<typename Sorter>
    static void SortAfterSign(Sorter & sorter)
    {
        T first_item = sorter.first_item();
        if (first_item == 0 || !SignBit(first_item))
        {
            SortByte<Size - 2>(sorter);
        }
        else
        {
            SortByteBackwards<Size - 2>(sorter);
        }
    }

    static bool SignBit(T f)
    {
        return (GetByte<Size - 1>(f) & 0x80) != 0;
    }
    template<size_t Index>
    static uint8_t GetByte(T f)
    {
        union
        {
            T f;
            std::uint8_t bytes[Size];
        }
        as_union = { f };
        static_assert(sizeof(T) == Size || (std::is_same_v<T, long double> && sizeof(T) >= Size));
        static_assert(Index < Size);
        return as_union.bytes[Index];
    }
    static uint8_t FirstByte(T f)
    {
        if (f == 0)
            return 0x80;
        std::uint8_t byte = GetByte<Size - 1>(f);
        std::uint8_t sign_bit = -std::int8_t(byte >> 7);
        return byte ^ (sign_bit | 0x80);
        // todo: on GCC the below code is faster because it turns this into
        // branchless code as well, using a cmovs instruction. I don't have
        // access to that instruction using C++, so I just have to trust the
        // compiler optimizer. unfortunately clang doesn't use cmovs here and
        // on a randomly shuffled array the code using branches is much slower
        // than the above code.
        // so I'll leave it at my custom branchless code above
        /*if (byte & 0x80)
            return ~byte;
        else
            return byte | 0x80;*/
    }

    template<typename Sorter>
    void operator()(Sorter & sorter)
    {
        /*sorter.sort_with_faster_comparison([](T f)
        {
            return FirstByte(f);
        }, CompareAsUint{}, &SortAfterSign<Sorter>);*/
        sorter.sort([](T f)
        {
            return FirstByte(f);
        }, &SortAfterSign<Sorter>);
    }
};



struct DefaultContainerAccess
{
    /*template<typename T>
    static decltype(auto) AccessAt(T && container, size_t index)
    {
        return container[index];
    }*/
    template<typename T>
    static decltype(auto) AccessAt(T && container, size_t index)
    {
        using std::begin;
        return *std::next(begin(container), index);
    }
    template<typename T>
    static size_t ContainerSize(T && container)
    {
        return container.size();
    }
};

template<typename T, typename Enable = void>
struct ContainerAccessDecisionDefaultImpl
{
    static constexpr bool UseDefaultImpl = false;
};
template<typename T>
struct ContainerAccessDecisionDefaultImpl<T, std::void_t<decltype(*std::declval<T>().begin())>>
{
    static constexpr bool UseDefaultImpl = true;
};
template<typename T, typename Enable = void>
struct ContainerAccessDecisionCustomImpl
{
    static constexpr bool UseCustomImpl = false;
};
template<typename T>
struct ContainerAccessDecisionCustomImpl<T, std::void_t<decltype(SkaSortContainerAccess<T>::AccessAt(std::declval<T>(), 0))>>
{
    static constexpr bool UseCustomImpl = true;
};

template<typename T>
struct ContainerAccessDecision
{
    using type = std::conditional_t<ContainerAccessDecisionCustomImpl<T>::UseCustomImpl, SkaSortContainerAccess<T>,
                 std::conditional_t<ContainerAccessDecisionDefaultImpl<T>::UseDefaultImpl, DefaultContainerAccess,
                 void>>;
};


template<typename T, typename Enable = void>
struct FallbackSkaSorter
{
    template<typename Sorter>
    void operator()(Sorter & sorter)
    {
        sorter.sort([](auto && v)
        {
            return to_radix_sort_key(v);
        });
    }
};

template<typename T>
struct FallbackSkaSorter<T, std::enable_if_t<!std::is_same_v<typename detail::ContainerAccessDecision<T>::type, void> && std::is_convertible_v<typename std::iterator_traits<decltype(std::declval<T>().begin())>::iterator_category, std::random_access_iterator_tag>>>
{
    using ContainerAccess = typename detail::ContainerAccessDecision<T>::type;

    struct ListSortData
    {
        size_t current_index = 0;
        size_t recursion_limit = 16;
    };

    struct SortAtIndex
    {
        size_t index = 0;
        bool operator()(const T & l, const T & r) const
        {
            return SkaSortContainerCompare<T>::LessThanAtIndex(l, r, index);
        }
    };

    template<typename Sorter>
    static size_t CommonPrefix(Sorter & sorter, size_t start_index)
    {
        const auto & largest_match_list = sorter.first_item();
        size_t largest_match = ContainerAccess::ContainerSize(largest_match_list);
        if (largest_match == start_index)
            return start_index;
        bool first = true;
        sorter.for_each_item([&](const auto & current_list)
        {
            if (first)
            {
                first = false;
                return true;
            }
            size_t current_size = ContainerAccess::ContainerSize(current_list);
            if (current_size < largest_match)
            {
                largest_match = current_size;
                if (largest_match == start_index)
                    return false;
            }
            if (ContainerAccess::AccessAt(largest_match_list, start_index) != ContainerAccess::AccessAt(current_list, start_index))
            {
                largest_match = start_index;
                return false;
            }
            for (size_t i = start_index + 1; i < largest_match; ++i)
            {
                if (ContainerAccess::AccessAt(largest_match_list, i) != ContainerAccess::AccessAt(current_list, i))
                {
                    largest_match = i;
                    break;
                }
            }
            return true;
        });
        return largest_match;
    }

    template<typename Sorter>
    static void sort(Sorter & sorter, ListSortData sort_data)
    {
        sort_data.current_index = CommonPrefix(sorter, sort_data.current_index);
        sorter.sort([current_index = sort_data.current_index](const auto & list)
        {
            return ContainerAccess::ContainerSize(list) > current_index;
        }, static_cast<void (*)(Sorter &, ListSortData)>([](Sorter & sorter, ListSortData sort_data)
        {
            if (ContainerAccess::ContainerSize(sorter.first_item()) <= sort_data.current_index)
                sorter.skip();
            else
            {
                sorter.sort_with_faster_comparison([current_index = sort_data.current_index](const auto & list) -> decltype(auto)
                {
                    return ContainerAccess::AccessAt(list, current_index);
                }, SortAtIndex{sort_data.current_index}, &sort_from_recursion<Sorter>, sort_data);
            }
        }), sort_data);
    }

    template<typename Sorter>
    static void sort_from_recursion(Sorter & sorter, ListSortData sort_data)
    {
        ++sort_data.current_index;
        --sort_data.recursion_limit;
        if (sort_data.recursion_limit == 0)
        {
            sorter.std_sort_fallback(SortAtIndex{sort_data.current_index});
        }
        else
        {
            sort(sorter, sort_data);
        }
    }

    template<typename Sorter>
    void operator()(Sorter & sorter)
    {
        sort(sorter, ListSortData());
    }
};

template<typename T>
struct FallbackSkaSorter<T, std::enable_if_t<!std::is_same_v<typename detail::ContainerAccessDecision<T>::type, void> && !std::is_convertible_v<typename std::iterator_traits<decltype(std::declval<T>().begin())>::iterator_category, std::random_access_iterator_tag> && std::is_convertible_v<typename std::iterator_traits<decltype(std::declval<T>().begin())>::iterator_category, std::forward_iterator_tag> > >
{
    using ContainerAccess = typename detail::ContainerAccessDecision<T>::type;

    using iterator_type = decltype(std::declval<T>().begin());

    struct SortProxy
    {
        SortProxy(T & to_sort, size_t original_index)
            : it(to_sort.begin())
            , end(to_sort.end())
            , original(std::addressof(to_sort))
            , original_index(original_index)
        {
        }

        iterator_type it;
        iterator_type end;
        T * original;
        size_t original_index;
    };

    struct ListSortData
    {
        size_t recursion_limit = 16;
    };

    struct SortFallback
    {
        bool operator()(const SortProxy & l, const SortProxy & r) const
        {
            return std::lexicographical_compare(l.it, l.end, r.it, r.end);
        }
    };

    template<typename Sorter>
    static void SkipCommonPrefix(Sorter & sorter)
    {
        const SortProxy & largest_match_list = sorter.first_item();
        if (largest_match_list.it == largest_match_list.end)
            return;
        bool first = true;
        size_t largest_match = std::numeric_limits<size_t>::max();
        sorter.for_each_item([&](const SortProxy & current_list)
        {
            if (first)
            {
                first = false;
                return true;
            }
            size_t match_size = 0;
            for (iterator_type it = current_list.it, end = current_list.end, it2 = largest_match_list.it, it2end = largest_match_list.end; it != end; ++it)
            {
                if (*it != *it2)
                    break;
                ++match_size;
                if (match_size == largest_match)
                    return true;
                ++it2;
                if (it2 == it2end)
                    break;
            }
            if (match_size < largest_match)
            {
                if (match_size == 0)
                {
                    largest_match = 0;
                    return false;
                }
                largest_match = match_size;
            }
            return true;
        });
        if (largest_match == 0)
            return;
        sorter.for_each_item([largest_match](SortProxy & current_list)
        {
            for (size_t i = 0; i < largest_match; ++i)
                ++current_list.it;
            return true;
        });
    }

    template<typename Sorter>
    static void sort(Sorter & sorter, ListSortData sort_data)
    {
        SkipCommonPrefix(sorter);
        sorter.sort([](const SortProxy & list)
        {
            return list.it != list.end;
        }, static_cast<void (*)(Sorter &, ListSortData)>([](Sorter & sorter, ListSortData sort_data)
        {
            const SortProxy & first_item = sorter.first_item();
            if (first_item.it == first_item.end)
                sorter.skip();
            else
            {
                sorter.sort([](const SortProxy & proxy){ return *proxy.it; }, &sort_from_recursion<Sorter>, sort_data);
            }
        }), sort_data);
    }

    template<typename Sorter>
    static void sort_from_recursion(Sorter & sorter, ListSortData sort_data)
    {
        sorter.for_each_item([](SortProxy & proxy)
        {
            ++proxy.it;
            return true;
        });
        --sort_data.recursion_limit;
        if (sort_data.recursion_limit == 0)
        {
            sorter.std_sort_fallback(SortFallback());
        }
        else
        {
            sort(sorter, sort_data);
        }
    }

    template<typename Sorter>
    void operator()(Sorter & sorter)
    {
        std::vector<SortProxy> sort_this_instead;
        sort_this_instead.reserve(sorter.num_items());
        size_t index = 0;
        sorter.for_each_item([&](T & to_sort)
        {
            sort_this_instead.emplace_back(to_sort, index);
            ++index;
            return true;
        });
        auto new_sorter = sorter.change_list_to_sort(sort_this_instead.begin(), sort_this_instead.end(), IdentityFunctor(), SortFallback());
        sort(new_sorter, ListSortData());
        for (size_t i = 0, end = sort_this_instead.size(); i < end; ++i)
        {
            size_t original_index = sort_this_instead[i].original_index;
            if (original_index == i)
                continue;
            T tmp = std::move(*sort_this_instead[i].original);
            size_t target_index = i;
            do
            {
                *sort_this_instead[target_index].original = std::move(*sort_this_instead[original_index].original);
                target_index = original_index;
                original_index = std::exchange(sort_this_instead[original_index].original_index, original_index);
            }
            while(original_index != i);
            *sort_this_instead[target_index].original = std::move(tmp);
        }
        sorter.skip();
    }
};


template<typename SortSettings, typename It, typename ExtractKey>
void ska_sort2_with_settings(It begin, It end, ExtractKey && extract_key)
{
    auto invoke_wrapper = [extract_key](auto && a) -> decltype(auto)
    {
        return std::invoke(extract_key, std::forward<decltype(a)>(a));
    };
    std::ptrdiff_t num_elements = end - begin;
    auto fallback_compare = [invoke_wrapper](auto && l, auto && r)
    {
        return invoke_wrapper(l) < invoke_wrapper(r);
    };
    if (DefaultSortIfLessThanThreshold<SortSettings>(begin, end, num_elements, fallback_compare))
        return;
    using Sorter = SkaSorter<std::decay_t<decltype(invoke_wrapper(*begin))>>;
    detail::SkaSortImpl<SortSettings, false, It, decltype(fallback_compare), decltype(invoke_wrapper)> impl{begin, end, fallback_compare, invoke_wrapper};
    Sorter()(impl);
}
template<typename SortSettings, typename It>
void ska_sort2_with_settings(It begin, It end)
{
    return ska_sort2_with_settings<SortSettings>(begin, end, IdentityFunctor());
}

}

template<typename T>
struct SkaSorter<T &> : SkaSorter<T>
{
};
template<typename T>
struct SkaSorter<T &&> : SkaSorter<T>
{
};
template<typename T>
struct SkaSorter<const T> : SkaSorter<T>
{
};
template<typename T>
struct SkaSorter<volatile T> : SkaSorter<T>
{
};
template<>
struct SkaSorter<unsigned char> : detail::SizedUnsignedSorter<sizeof(unsigned char)>
{
};
template<>
struct SkaSorter<unsigned short> : detail::SizedUnsignedSorter<sizeof(unsigned short)>
{
};
template<>
struct SkaSorter<unsigned int> : detail::SizedUnsignedSorter<sizeof(unsigned int)>
{
};
template<>
struct SkaSorter<unsigned long> : detail::SizedUnsignedSorter<sizeof(unsigned long)>
{
};
template<>
struct SkaSorter<unsigned long long> : detail::SizedUnsignedSorter<sizeof(unsigned long long)>
{
};
template<>
struct SkaSorter<bool>
{
    template<typename Sorter>
    void operator()(Sorter & sorter)
    {
        sorter.sort([](bool value)
        {
            return value;
        });
    }
};
template<>
struct SkaSorter<char> : detail::ByValueToUnsignedSorter
{
};
template<>
struct SkaSorter<signed char> : detail::ByValueToUnsignedSorter
{
};
template<>
struct SkaSorter<char16_t> : detail::ByValueToUnsignedSorter
{
};
template<>
struct SkaSorter<char32_t> : detail::ByValueToUnsignedSorter
{
};
template<>
struct SkaSorter<signed short> : detail::ByValueToUnsignedSorter
{
};
template<>
struct SkaSorter<signed int> : detail::ByValueToUnsignedSorter
{
};
template<>
struct SkaSorter<signed long> : detail::ByValueToUnsignedSorter
{
};
template<>
struct SkaSorter<signed long long> : detail::ByValueToUnsignedSorter
{
};
template<typename T>
struct SkaSorter<T *> : detail::ByValueToUnsignedSorter
{
};
template<>
struct SkaSorter<float> : detail::FloatSorter<float>
{
};
template<>
struct SkaSorter<double> : detail::FloatSorter<double>
{
};
template<>
struct SkaSorter<long double> : detail::FloatSorter<long double, 10>
{
};
template<typename K, typename V>
struct SkaSorter<std::pair<K, V>>
{
    template<typename Sorter>
    static void SortSecond(Sorter & sorter)
    {
        sorter.sort([](const std::pair<K, V> & value) -> const V &
        {
            return value.second;
        });
    }

    template<typename Sorter>
    void operator()(Sorter & sorter)
    {
        sorter.sort([](const std::pair<K, V> & value) -> const K &
        {
            return value.first;
        }, &SortSecond<Sorter>);
    }
};

template<typename... Args>
struct SkaSorter<std::tuple<Args...>>
{
    template<size_t Index, typename Sorter>
    static void SortAtIndex(Sorter & sorter)
    {
        if constexpr (Index + 1 == std::tuple_size_v<std::tuple<Args...>>)
        {
            sorter.sort([](const std::tuple<Args...> & value) -> decltype(auto)
            {
                return std::get<Index>(value);
            });
        }
        else
        {
            sorter.sort([](const std::tuple<Args...> & value) -> decltype(auto)
            {
                return std::get<Index>(value);
            }, &SortAtIndex<Index + 1, Sorter>);
        }
    }
    template<typename Sorter>
    void operator()(Sorter & sorter)
    {
        SortAtIndex<0>(sorter);
    }
};
template<typename T>
struct SkaSorter<std::optional<T>>
{
    template<typename Sorter>
    static void SortAfterBool(Sorter & sorter)
    {
        if (sorter.first_item().has_value())
        {
            sorter.sort([](const std::optional<T> & value) -> const T &
            {
                return *value;
            });
        }
        else
            sorter.skip();
    }
    template<typename Sorter>
    void operator()(Sorter & sorter)
    {
        sorter.sort([](const std::optional<T> & value) -> bool
        {
            return value.has_value();
        }, &SortAfterBool<Sorter>);
    }
};
template<typename... Args>
struct SkaSorter<std::variant<Args...>>
{
    template<typename>
    struct IndexedSorter;
    template<size_t... Indices>
    struct IndexedSorter<std::integer_sequence<size_t, Indices...>>
    {
        template<size_t Index, typename Sorter>
        static void SortSubindex(Sorter & sorter)
        {
            sorter.sort([](const std::variant<Args...> & value) -> decltype(auto)
            {
                return std::get<Index>(value);
            });
        }

        template<typename Sorter>
        static void SortAfterIndex(Sorter & sorter)
        {
            size_t index = sorter.first_item().index();
            if (index == std::variant_npos)
            {
                sorter.skip();
                return;
            }
            static constexpr void (*subindex_sorts[sizeof...(Args)])(Sorter &) =
            {
                &SortSubindex<Indices, Sorter>...,
            };
            subindex_sorts[index](sorter);
        }
    };
    template<typename Sorter>
    void operator()(Sorter & sorter)
    {
        sorter.sort([](const std::variant<Args...> & value)
        {
            if constexpr (sizeof...(Args) < std::numeric_limits<std::uint8_t>::max())
                return static_cast<std::uint8_t>(value.index() + 1);
            else if constexpr(sizeof...(Args) < std::numeric_limits<std::uint16_t>::max())
                return static_cast<std::uint16_t>(value.index() + 1);
            else if constexpr(sizeof...(Args) < std::numeric_limits<std::uint32_t>::max())
                return static_cast<std::uint32_t>(value.index() + 1);
            else
                return value.index() + 1;
        }, &IndexedSorter<std::make_index_sequence<sizeof...(Args)>>::template SortAfterIndex<Sorter>);
    }
};

template<typename T>
struct SkaSortContainerCompare
{
    static bool LessThanAtIndex(const T & l, const T & r, size_t index)
    {
        using std::begin;
        using std::end;
        return std::lexicographical_compare(std::next(begin(l), index), end(l), std::next(begin(r), index), end(r));
    }
};

template<typename C, typename T, typename A>
struct SkaSortContainerCompare<std::basic_string<C, T, A>>
{
    using StringType = std::basic_string<C, T, A>;
    /*static bool LessThanAtIndex(const StringType & l, const StringType & r, size_t index)
    {
        return l.compare(index, StringType::npos, r, index) < 0;
    }*/
    static bool LessThanAtIndex(const StringType & l, const StringType & r, size_t index)
    {
        size_t l_size = l.size();
        size_t r_size = r.size();
        bool l_is_smaller = l_size < r_size;
        int compare_result = T::compare(l.data() + index, r.data() + index, (l_is_smaller ? l_size : r_size) - index);
        if (compare_result == 0)
            return l_is_smaller;
        else
            return compare_result < 0;
    }
    /*static bool LessThanAtIndex(const StringType & l, const StringType & r, size_t index)
    {
        std::basic_string_view<C, T> l_view(l.data() + index, l.size() - index);
        std::basic_string_view<C, T> r_view(r.data() + index, r.size() - index);
        return l_view < r_view;
    }*/
    /*static bool LessThanAtIndex(const StringType & l, const StringType & r, size_t)
    {
        return l < r;
    }*/
};

template<typename C, typename T>
struct SkaSortContainerCompare<std::basic_string_view<C, T>>
{
    using StringType = std::basic_string_view<C, T>;
    static bool LessThanAtIndex(StringType l, StringType r, size_t index)
    {
        size_t l_size = l.size();
        size_t r_size = r.size();
        bool l_is_smaller = l_size < r_size;
        int compare_result = T::compare(l.data() + index, r.data() + index, (l_is_smaller ? l_size : r_size) - index);
        if (compare_result == 0)
            return l_is_smaller;
        else
            return compare_result < 0;
    }
};

template<typename T>
struct SkaSorter : detail::FallbackSkaSorter<T>
{
};

template<typename It, typename ExtractKey>
void ska_sort2(It begin, It end, ExtractKey && extract_key)
{
    detail::ska_sort2_with_settings<detail::DefaultSortSettings>(begin, end, std::forward<ExtractKey>(extract_key));
}
template<typename It>
void ska_sort2(It begin, It end)
{
    return ska_sort2(begin, end, detail::IdentityFunctor());
}
