#pragma once

#include <memory>
#include <vector>
#include <iterator>
#include <algorithm>
#include "debug/assert.hpp"

template<typename T, size_t SmallSize, typename Allocator>
struct small_inplace_vector;

namespace detail
{
template<typename T, size_t SmallSize, typename Allocator>
struct small_vector_typedef
{
    typedef small_inplace_vector<T, SmallSize, Allocator> type;
};
template<typename T, typename Allocator>
struct small_vector_typedef<T, 0, Allocator>
{
    typedef std::vector<T, Allocator> type;
};
}

template<typename T, size_t SmallSize, typename Allocator = std::allocator<T>>
using small_vector = typename detail::small_vector_typedef<T, SmallSize, Allocator>::type;

template<typename T, size_t SmallSize, typename Allocator>
struct small_inplace_vector : private Allocator
{
    static constexpr size_t NumInplace = std::max(SmallSize, sizeof(void *) / sizeof(T));
private:
    T * _begin;
    T * _end;
    union
    {
        T * _capacity;
        T inplace[NumInplace];
    };

    Allocator & alloc()
    {
        return *this;
    }
    bool is_using_inplace() const
    {
        return _begin == inplace;
    }

public:
    small_inplace_vector()
        : _begin(inplace), _end(inplace)
    {
    }
    explicit small_inplace_vector(size_t count)
        : small_inplace_vector()
    {
        reserve(count);
        for (; count > 0; --count)
        {
            emplace_back_no_check();
        }
    }
    template<typename It>
    small_inplace_vector(It begin, It end)
        : small_inplace_vector()
    {
        insert(_begin, begin, end);
    }
    small_inplace_vector(std::initializer_list<T> il)
        : small_inplace_vector(il.begin(), il.end())
    {
    }
    small_inplace_vector(const small_inplace_vector & other)
        : small_inplace_vector(other.begin(), other.end())
    {
    }
    small_inplace_vector(small_inplace_vector && other)
        : small_inplace_vector()
    {
        if (other.is_using_inplace())
        {
            insert(_end, std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
        }
        else
        {
            _begin = other._begin;
            _end = other._end;
            _capacity = other._capacity;
            other._begin = other._end = other.inplace;
        }
    }
    small_inplace_vector & operator=(const small_inplace_vector & other)
    {
        small_inplace_vector(other).swap(*this);
    }
    small_inplace_vector & operator=(small_inplace_vector && other)
    {
        swap(other);
        return *this;
    }
    ~small_inplace_vector()
    {
        clear();
        if (!is_using_inplace())
            std::allocator_traits<Allocator>::deallocate(alloc(), _begin, capacity());
    }
    Allocator get_allocator() const
    {
        return alloc();
    }
    T & at(size_t index)
    {
        DE_ASSER(index < size());
        return _begin[index];
    }
    const T & at(size_t index) const
    {
        return const_cast<small_inplace_vector &>(*this).at(index);
    }
    T & operator[](size_t index)
    {
        return _begin[index];
    }
    const T & operator[](size_t index) const
    {
        return _begin[index];
    }
    T & front()
    {
        return _begin[0];
    }
    const T & front() const
    {
        return _begin[0];
    }
    T & back()
    {
        return _end[-1];
    }
    const T & back() const
    {
        return _end[-1];
    }
    T * data()
    {
        return _begin;
    }
    const T * data() const
    {
        return _begin;
    }
    typedef T * iterator;
    typedef const T * const_iterator;
    typedef std::reverse_iterator<T *> reverse_iterator;
    typedef std::reverse_iterator<const T *> const_reverse_iterator;
    iterator begin() { return _begin; }
    const_iterator begin() const { return _begin; }
    const_iterator cbegin() const { return _begin; }
    iterator end() { return _end; }
    const_iterator end() const { return _end; }
    const_iterator cend() const { return _end; }
    reverse_iterator rbegin() { return reverse_iterator(_end); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(_end); }
    const_reverse_iterator crbegin() const { return const_reverse_iterator(_end); }
    reverse_iterator rend() { return reverse_iterator(_begin); }
    const_reverse_iterator rend() const { return const_reverse_iterator(_begin); }
    const_reverse_iterator crend() const { return const_reverse_iterator(_begin); }

    size_t capacity() const
    {
        if (is_using_inplace())
            return NumInplace;
        else
            return _capacity - _begin;
    }
    bool empty() const
    {
        return _begin == _end;
    }
    size_t size() const
    {
        return _end - _begin;
    }
    size_t max_size() const
    {
        return size_t(-1);
    }
    void reserve(size_t size)
    {
        size_t my_capacity = capacity();
        if (size <= my_capacity)
            return;
        for (my_capacity *= 2; my_capacity < size; my_capacity *= 2);
        reserve_exactly(my_capacity);
    }
    void shrink_to_fit()
    {
        if (is_using_inplace())
            return;
        small_inplace_vector replacement;
        size_t my_size = size();
        if (my_size > NumInplace)
            replacement.reserve_exactly(my_size);
        replacement.insert(replacement._end, std::make_move_iterator(_begin), std::make_move_iterator(_end));
        replacement.swap(*this);
    }
    void clear()
    {
        for (T & element : *this)
        {
            std::allocator_traits<Allocator>::destroy(alloc(), std::addressof(element));
        }
        _end = _begin;
    }
    void insert(const_iterator pos, const T & value)
    {
        emplace(pos, value);
    }
    void insert(const_iterator pos, T && value)
    {
        emplace(pos, std::move(value));
    }
    template<typename It>
    void insert(const_iterator pos, It begin, It end)
    {
        size_t required_size = std::distance(begin, end) + size();
        if (required_size > capacity())
        {
            small_inplace_vector replacement;
            replacement.reserve(required_size);
            replacement.insert(replacement._end, std::make_move_iterator(_begin), std::make_move_iterator(const_cast<iterator>(pos)));
            replacement.insert(replacement._end, begin, end);
            replacement.insert(replacement._end, std::make_move_iterator(const_cast<iterator>(pos)), std::make_move_iterator(_end));
            swap(replacement);
        }
        else
        {
            auto old_end = _end;
            for (; begin != end; ++begin)
            {
                emplace_back_no_check(*begin);
            }
            std::rotate(const_cast<iterator>(pos), old_end, _end);
        }
    }
    void insert(const_iterator pos, std::initializer_list<T> il)
    {
        insert(pos, il.begin(), il.end());
    }
    template<typename... Args>
    void emplace(const_iterator pos, Args &&... args)
    {
        size_t insert_index = pos - _begin;
        emplace_back(std::forward<Args>(args)...);
        std::rotate(_begin + insert_index, _end - 1, _end);
    }
    template<typename It>
    void assign(It begin, It end)
    {
        auto it = _begin;
        auto my_end = _end;
        while (it != my_end && begin != end)
        {
            *it++ = *begin++;
        }
        if (it == my_end)
            insert(my_end, begin, end);
        else if (begin == end)
            erase(it, my_end);
    }
    void assign(std::initializer_list<T> il)
    {
        assign(il.begin(), il.end());
    }

    void erase(iterator pos)
    {
        erase(pos, pos + 1);
    }
    void erase(const_iterator pos)
    {
        erase(const_cast<iterator>(pos));
    }
    void erase(iterator first, iterator last)
    {
        if (first == last)
            return;
        std::move(last, _end, first);
        for (; first != last; ++first)
            pop_back();
    }
    void erase(const_iterator first, const_iterator last)
    {
        erase(const_cast<iterator>(first), const_cast<iterator>(last));
    }
    void push_back(const T & value)
    {
        emplace_back(value);
    }
    void push_back(T && value)
    {
        emplace_back(std::move(value));
    }
    template<typename... Args>
    void emplace_back(Args &&... args)
    {
        T value(std::forward<Args>(args)...);
        if (size() == capacity())
            reserve(capacity() * 2);
        emplace_back_no_check(std::move(value));
    }
    void pop_back()
    {
        std::allocator_traits<Allocator>::destroy(alloc(), _end - 1);
        --_end;
    }
    void resize(size_t new_size)
    {
        size_t old_size = size();
        if (new_size < old_size)
        {
            do
            {
                pop_back();
                --old_size;
            }
            while (old_size > new_size);
        }
        else if (new_size > old_size)
        {
            reserve(new_size);
            do
            {
                emplace_back_no_check();
                ++old_size;
            }
            while (old_size < new_size);
        }
    }

    void swap(small_inplace_vector & other)
    {
        if (is_using_inplace())
        {
            if (other.is_using_inplace())
            {
                auto it = _begin, oit = other._begin, end = _end, oend = other._end;
                for (; it != end && oit != oend; ++it, ++oit)
                {
                    std::iter_swap(it, oit);
                }
                if (it != end)
                {
                    other.insert(other.end(), std::make_move_iterator(it), std::make_move_iterator(end));
                    erase(it, end);
                }
                if (oit != oend)
                {
                    insert(end, std::make_move_iterator(oit), std::make_move_iterator(oend));
                    other.erase(oit, oend);
                }
            }
            else
            {
                auto oit = other.inplace;
                T * new_capacity = other._capacity;
                for (T & element : *this)
                {
                    std::allocator_traits<Allocator>::construct(other.alloc(), oit++, std::move(element));
                    std::allocator_traits<Allocator>::destroy(alloc(), std::addressof(element));
                }
                _begin = other._begin;
                _end = other._end;
                _capacity = new_capacity;
                other._begin = other.inplace;
                other._end = oit;
            }
        }
        else if (other.is_using_inplace())
        {
            auto it = inplace;
            T * new_capacity = _capacity;
            for (T & element : other)
            {
                std::allocator_traits<Allocator>::construct(alloc(), it++, std::move(element));
                std::allocator_traits<Allocator>::destroy(other.alloc(), std::addressof(element));
            }
            other._begin = _begin;
            other._end = _end;
            other._capacity = new_capacity;
            _begin = inplace;
            _end = it;
        }
        else
        {
            std::swap(_begin, other._begin);
            std::swap(_end, other._end);
            std::swap(_capacity, other._capacity);
        }
    }

private:

    void reserve_exactly(size_t size)
    {
        small_inplace_vector replacement;
        replacement._begin = replacement._end = std::allocator_traits<Allocator>::allocate(alloc(), size);
        replacement._capacity = replacement._begin + size;
        for (T & element : *this)
            replacement.emplace_back_no_check(std::move(element));
        swap(replacement);
    }
    template<typename... Args>
    void emplace_back_no_check(Args &&... args)
    {
        std::allocator_traits<Allocator>::construct(alloc(), _end, std::forward<Args>(args)...);
        ++_end;
    }
};

template<typename T, size_t SmallSize, typename Allocator>
bool operator==(const small_inplace_vector<T, SmallSize, Allocator> & lhs, const small_inplace_vector<T, SmallSize, Allocator> & rhs)
{
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}
template<typename T, size_t SmallSize, typename Allocator>
bool operator!=(const small_inplace_vector<T, SmallSize, Allocator> & lhs, const small_inplace_vector<T, SmallSize, Allocator> & rhs)
{
    return !(lhs == rhs);
}
template<typename T, size_t SmallSize, typename Allocator>
bool operator<(const small_inplace_vector<T, SmallSize, Allocator> & lhs, const small_inplace_vector<T, SmallSize, Allocator> & rhs)
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}
template<typename T, size_t SmallSize, typename Allocator>
bool operator>(const small_inplace_vector<T, SmallSize, Allocator> & lhs, const small_inplace_vector<T, SmallSize, Allocator> & rhs)
{
    return rhs < lhs;
}
template<typename T, size_t SmallSize, typename Allocator>
bool operator<=(const small_inplace_vector<T, SmallSize, Allocator> & lhs, const small_inplace_vector<T, SmallSize, Allocator> & rhs)
{
    return !(rhs < lhs);
}
template<typename T, size_t SmallSize, typename Allocator>
bool operator>=(const small_inplace_vector<T, SmallSize, Allocator> & lhs, const small_inplace_vector<T, SmallSize, Allocator> & rhs)
{
    return !(lhs < rhs);
}
template<typename T, size_t SmallSize, typename Allocator>
void swap(small_inplace_vector<T, SmallSize, Allocator> & lhs, small_inplace_vector<T, SmallSize, Allocator> & rhs)
{
    lhs.swap(rhs);
}

