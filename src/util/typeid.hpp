#pragma once

#include <cstddef>

struct custom_typeinfo
{
    custom_typeinfo() = default;

    template<typename T>
    static custom_typeinfo create()
    {
        return custom_typeinfo(reinterpret_cast<void *>(&create<T>));
    }

    bool operator==(const custom_typeinfo & other) const
    {
        return ptr == other.ptr;
    }
    bool operator!=(const custom_typeinfo & other) const
    {
        return !(*this == other);
    }
    bool operator<(const custom_typeinfo & other) const
    {
        return ptr < other.ptr;
    }
    bool operator>(const custom_typeinfo & other) const
    {
        return other < *this;
    }
    bool operator<=(const custom_typeinfo & other) const
    {
        return !(other < *this);
    }
    bool operator>=(const custom_typeinfo & other) const
    {
        return !(*this < other);
    }

private:
    custom_typeinfo(void * ptr)
        : ptr(ptr)
    {
    }

    template<typename>
    friend struct std::hash;
    void * ptr = nullptr;
};

template<typename T>
custom_typeinfo custom_typeid()
{
    return custom_typeinfo::create<T>();
}

namespace std
{
template<typename>
struct hash;
template<>
struct hash<custom_typeinfo>
{
    size_t operator()(custom_typeinfo type) const
    {
        return reinterpret_cast<size_t>(type.ptr);
    }
};
}
