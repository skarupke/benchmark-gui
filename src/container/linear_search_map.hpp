#pragma once

#include <vector>
#include <algorithm>
#include <functional>

namespace ska
{

namespace detail_inplace_linear_search
{
template<typename Result, typename Functor>
struct functor_storage : Functor
{
    functor_storage() = default;
    functor_storage(const Functor & functor)
        : Functor(functor)
    {
    }
    template<typename... Args>
    Result operator()(Args &&... args)
    {
        return static_cast<Functor &>(*this)(std::forward<Args>(args)...);
    }
    template<typename... Args>
    Result operator()(Args &&... args) const
    {
        return static_cast<const Functor &>(*this)(std::forward<Args>(args)...);
    }
};
template<typename Result, typename... Args>
struct functor_storage<Result, Result (*)(Args...)>
{
    typedef Result (*function_ptr)(Args...);
    function_ptr function;
    functor_storage(function_ptr function)
        : function(function)
    {
    }
    Result operator()(Args... args) const
    {
        return function(std::forward<Args>(args)...);
    }
    operator function_ptr &()
    {
        return function;
    }
    operator const function_ptr &()
    {
        return function;
    }
};
template<typename key_type, typename value_type, typename key_equal>
struct KeyOrValueEquality : functor_storage<bool, key_equal>
{
    typedef functor_storage<bool, key_equal> equality_storage;
    KeyOrValueEquality() = default;
    KeyOrValueEquality(const key_equal & equality)
        : equality_storage(equality)
    {
    }
    bool operator()(const key_type & lhs, const key_type & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs, rhs);
    }
    bool operator()(const key_type & lhs, const value_type & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs, rhs.first);
    }
    bool operator()(const value_type & lhs, const key_type & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs);
    }
    bool operator()(const value_type & lhs, const value_type & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs.first);
    }
    template<typename F, typename S>
    bool operator()(const key_type & lhs, const std::pair<F, S> & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs, rhs.first);
    }
    template<typename F, typename S>
    bool operator()(const std::pair<F, S> & lhs, const key_type & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs);
    }
    template<typename F, typename S>
    bool operator()(const value_type & lhs, const std::pair<F, S> & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs.first);
    }
    template<typename F, typename S>
    bool operator()(const std::pair<F, S> & lhs, const value_type & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs.first);
    }
    template<typename FL, typename SL, typename FR, typename SR>
    bool operator()(const std::pair<FL, SL> & lhs, const std::pair<FR, SR> & rhs)
    {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs.first);
    }
};
}

template<typename K, typename V, typename A = std::allocator<std::pair<K, V>>>
struct linear_search_map
{
private:
    using vec_type = std::vector<std::pair<K, V>, A>;
public:
    using iterator = typename vec_type::iterator;
    using const_iterator = typename vec_type::const_iterator;
    using key_type = K;
    using mapped_type = V;
    using value_type = std::pair<K, V>;

    template<typename Key, typename... Args>
    std::pair<iterator, bool> emplace(Key && key, Args &&... args)
    {
        detail_inplace_linear_search::KeyOrValueEquality<K, std::pair<K, V>, std::equal_to<K>> compare;
        for (iterator it = vec.begin(), end = vec.end(); it != end; ++it)
        {
            if (compare(key, *it))
                return { it, false };
        }
        vec.emplace_back(std::forward<Key>(key), std::forward<Args>(args)...);
        return { vec.end() - 1, true };
    }
    V & operator[](const K & key)
    {
        return emplace(key, convertible_to_value()).first->second;
    }
    std::pair<iterator, bool> insert(const value_type & value)
    {
        return emplace(value);
    }
    std::pair<iterator, bool> insert(value_type && value)
    {
        return emplace(std::move(value));
    }
    template<typename It>
    void insert(It begin, It end)
    {
        for (; begin != end; ++begin)
            insert(*begin);
    }

    iterator find(const K & key)
    {
        detail_inplace_linear_search::KeyOrValueEquality<K, std::pair<K, V>, std::equal_to<K>> compare;
        for (iterator it = vec.begin(), end = vec.end(); it != end; ++it)
        {
            if (compare(key, *it))
                return it;
        }
        return vec.end();
    }
    const_iterator find(const K & key) const
    {
        return const_cast<linear_search_map &>(*this).find(key);
    }

    iterator erase(const_iterator it)
    {
        iterator as_non_const = vec.begin() + (it - vec.cbegin());
        *as_non_const = std::move(vec.back());
        vec.pop_back();
        return as_non_const;
    }
    iterator erase(const_iterator begin, const_iterator end)
    {
        return vec.erase(begin, end);
    }

    size_t erase(const K & key)
    {
        auto found = find(key);
        if (found == end())
            return 0;
        else
        {
            erase(found);
            return 1;
        }
    }

    iterator begin()
    {
        return vec.begin();
    }
    iterator end()
    {
        return vec.end();
    }
    const_iterator begin() const
    {
        return vec.begin();
    }
    const_iterator end() const
    {
        return vec.end();
    }
    const_iterator cbegin() const
    {
        return begin();
    }
    const_iterator cend() const
    {
        return end();
    }

    size_t size() const
    {
        return vec.size();
    }
    void reserve(size_t size)
    {
        vec.reserve(size);
    }

private:
    vec_type vec;


    struct convertible_to_value
    {
        operator V() const
        {
            return V();
        }
    };
};

}
