#pragma once

#include <memory>
#include <cstddef>

template<typename T, typename S>
struct TwoWayPointer
{
private:
    TwoWayPointer<S, T> * ptr;
    template<typename, typename>
    friend struct TwoWayPointer;
    template<typename T2, typename S2>
    friend bool operator==(const TwoWayPointer<T2, S2> &, const TwoWayPointer<T2, S2> &);
    template<typename T2, typename S2>
    friend bool operator!=(const TwoWayPointer<T2, S2> &, const TwoWayPointer<T2, S2> &);
    template<typename T2, typename S2>
    friend bool operator==(const TwoWayPointer<T2, S2> &, std::nullptr_t);
    template<typename T2, typename S2>
    friend bool operator!=(const TwoWayPointer<T2, S2> &, std::nullptr_t);
    template<typename T2, typename S2>
    friend bool operator==(std::nullptr_t, const TwoWayPointer<T2, S2> &);
    template<typename T2, typename S2>
    friend bool operator!=(std::nullptr_t, const TwoWayPointer<T2, S2> &);

public:
    T value;

    TwoWayPointer()
        : ptr(), value()
    {
    }
    TwoWayPointer(T value)
        : ptr(), value(std::move(value))
    {
    }
    TwoWayPointer(TwoWayPointer<S, T> * ptr)
        : ptr(ptr), value()
    {
        if (ptr)
            ptr->ptr = this;
    }
    TwoWayPointer(TwoWayPointer<S, T> * ptr, T value)
        : ptr(ptr), value(std::move(value))
    {
        if (ptr)
            ptr->ptr = this;
    }

    TwoWayPointer(TwoWayPointer && other)
        : TwoWayPointer(other.ptr, std::move(other.value))
    {
        other.ptr = nullptr;
    }
    TwoWayPointer & operator=(TwoWayPointer && other)
    {
        if (ptr)
        {
            if (other.ptr)
                std::swap(ptr->ptr, other.ptr->ptr);
            else
                ptr->ptr = &other;
        }
        else if (other.ptr)
            other.ptr->ptr = this;
        std::swap(ptr, other.ptr);
        value = std::move(other.value);
        return *this;
    }
    TwoWayPointer & operator=(TwoWayPointer<S, T> * ptr)
    {
        return *this = TwoWayPointer(ptr);
    }
    ~TwoWayPointer()
    {
        cut_connection();
    }

    void cut_connection()
    {
        if (ptr)
        {
            ptr->ptr = nullptr;
            ptr = nullptr;
        }
    }

    S * operator->() const
    {
        return std::addressof(ptr->value);
    }
    S & operator*() const
    {
        return ptr->value;
    }

    explicit operator bool() const
    {
        return ptr;
    }
    bool operator!() const
    {
        return !ptr;
    }
};
template<typename S>
struct TwoWayPointer<void, S>
{
private:
    TwoWayPointer<S, void> * ptr;
    template<typename, typename>
    friend struct TwoWayPointer;
    template<typename T2, typename S2>
    friend bool operator==(const TwoWayPointer<T2, S2> &, const TwoWayPointer<T2, S2> &);
    template<typename T2, typename S2>
    friend bool operator!=(const TwoWayPointer<T2, S2> &, const TwoWayPointer<T2, S2> &);
    template<typename T2, typename S2>
    friend bool operator==(const TwoWayPointer<T2, S2> &, std::nullptr_t);
    template<typename T2, typename S2>
    friend bool operator!=(const TwoWayPointer<T2, S2> &, std::nullptr_t);
    template<typename T2, typename S2>
    friend bool operator==(std::nullptr_t, const TwoWayPointer<T2, S2> &);
    template<typename T2, typename S2>
    friend bool operator!=(std::nullptr_t, const TwoWayPointer<T2, S2> &);

public:

    TwoWayPointer()
        : ptr()
    {
    }
    TwoWayPointer(TwoWayPointer<S, void> * ptr)
        : ptr(ptr)
    {
        if (ptr)
            ptr->ptr = this;
    }

    TwoWayPointer(TwoWayPointer && other)
        : TwoWayPointer(other.ptr)
    {
        other.ptr = nullptr;
    }
    TwoWayPointer & operator=(TwoWayPointer && other)
    {
        if (ptr)
        {
            if (other.ptr)
                std::swap(ptr->ptr, other.ptr->ptr);
            else
                ptr->ptr = &other;
        }
        else if (other.ptr)
            other.ptr->ptr = this;
        std::swap(ptr, other.ptr);
        return *this;
    }
    TwoWayPointer & operator=(TwoWayPointer<S, void> * ptr)
    {
        return *this = TwoWayPointer(ptr);
    }
    ~TwoWayPointer()
    {
        cut_connection();
    }

    void cut_connection()
    {
        if (ptr)
        {
            ptr->ptr = nullptr;
            ptr = nullptr;
        }
    }

    S * operator->() const
    {
        return std::addressof(ptr->value);
    }
    S & operator*() const
    {
        return ptr->value;
    }

    explicit operator bool() const
    {
        return ptr;
    }
    bool operator!() const
    {
        return !ptr;
    }
};
template<typename T>
struct TwoWayPointer<T, void>
{
private:
    TwoWayPointer<void, T> * ptr;
    template<typename, typename>
    friend struct TwoWayPointer;
    template<typename T2, typename S2>
    friend bool operator==(const TwoWayPointer<T2, S2> &, const TwoWayPointer<T2, S2> &);
    template<typename T2, typename S2>
    friend bool operator!=(const TwoWayPointer<T2, S2> &, const TwoWayPointer<T2, S2> &);
    template<typename T2, typename S2>
    friend bool operator==(const TwoWayPointer<T2, S2> &, std::nullptr_t);
    template<typename T2, typename S2>
    friend bool operator!=(const TwoWayPointer<T2, S2> &, std::nullptr_t);
    template<typename T2, typename S2>
    friend bool operator==(std::nullptr_t, const TwoWayPointer<T2, S2> &);
    template<typename T2, typename S2>
    friend bool operator!=(std::nullptr_t, const TwoWayPointer<T2, S2> &);

public:
    T value;

    TwoWayPointer()
        : ptr(), value()
    {
    }
    TwoWayPointer(T value)
        : ptr(), value(std::move(value))
    {
    }
    TwoWayPointer(TwoWayPointer<void, T> * ptr)
        : ptr(ptr), value()
    {
        if (ptr)
            ptr->ptr = this;
    }
    TwoWayPointer(TwoWayPointer<void, T> * ptr, T value)
        : ptr(ptr), value(std::move(value))
    {
        if (ptr)
            ptr->ptr = this;
    }

    TwoWayPointer(TwoWayPointer && other)
        : TwoWayPointer(other.ptr, std::move(other.value))
    {
        other.ptr = nullptr;
    }
    TwoWayPointer & operator=(TwoWayPointer && other)
    {
        if (ptr)
        {
            if (other.ptr)
                std::swap(ptr->ptr, other.ptr->ptr);
            else
                ptr->ptr = &other;
        }
        else if (other.ptr)
            other.ptr->ptr = this;
        std::swap(ptr, other.ptr);
        value = std::move(other.value);
        return *this;
    }
    TwoWayPointer & operator=(TwoWayPointer<void, T> * ptr)
    {
        return *this = TwoWayPointer(ptr);
    }
    ~TwoWayPointer()
    {
        cut_connection();
    }

    void cut_connection()
    {
        if (ptr)
        {
            ptr->ptr = nullptr;
            ptr = nullptr;
        }
    }

    explicit operator bool() const
    {
        return ptr;
    }
    bool operator!() const
    {
        return !ptr;
    }
};
template<>
struct TwoWayPointer<void, void>
{
private:
    TwoWayPointer<void, void> * ptr;
    template<typename, typename>
    friend struct TwoWayPointer;
    template<typename T2, typename S2>
    friend bool operator==(const TwoWayPointer<T2, S2> &, const TwoWayPointer<T2, S2> &);
    template<typename T2, typename S2>
    friend bool operator!=(const TwoWayPointer<T2, S2> &, const TwoWayPointer<T2, S2> &);
    template<typename T2, typename S2>
    friend bool operator==(const TwoWayPointer<T2, S2> &, std::nullptr_t);
    template<typename T2, typename S2>
    friend bool operator!=(const TwoWayPointer<T2, S2> &, std::nullptr_t);
    template<typename T2, typename S2>
    friend bool operator==(std::nullptr_t, const TwoWayPointer<T2, S2> &);
    template<typename T2, typename S2>
    friend bool operator!=(std::nullptr_t, const TwoWayPointer<T2, S2> &);

public:
    TwoWayPointer()
        : ptr()
    {
    }
    TwoWayPointer(TwoWayPointer<void, void> * ptr)
        : ptr(ptr)
    {
        if (ptr)
            ptr->ptr = this;
    }
    TwoWayPointer(TwoWayPointer && other)
        : TwoWayPointer(other.ptr)
    {
        other.ptr = nullptr;
    }
    TwoWayPointer & operator=(TwoWayPointer && other)
    {
        if (ptr)
            ptr->ptr = &other;
        std::swap(ptr, other.ptr);
        if (ptr)
            ptr->ptr = this;
        return *this;
    }
    TwoWayPointer & operator=(TwoWayPointer<void, void> * ptr)
    {
        return *this = TwoWayPointer(ptr);
    }
    ~TwoWayPointer()
    {
        cut_connection();
    }

    void cut_connection()
    {
        if (ptr)
        {
            ptr->ptr = nullptr;
            ptr = nullptr;
        }
    }

    explicit operator bool() const
    {
        return ptr;
    }
    bool operator!() const
    {
        return !ptr;
    }
};
template<typename T, typename S>
bool operator==(const TwoWayPointer<T, S> & lhs, const TwoWayPointer<T, S> & rhs)
{
    return lhs.ptr == rhs.ptr;
}
template<typename T, typename S>
bool operator!=(const TwoWayPointer<T, S> & lhs, const TwoWayPointer<T, S> & rhs)
{
    return !(lhs == rhs);
}
template<typename T, typename S>
bool operator==(const TwoWayPointer<T, S> & lhs, std::nullptr_t)
{
    return !lhs;
}
template<typename T, typename S>
bool operator!=(const TwoWayPointer<T, S> & lhs, std::nullptr_t)
{
    return !(lhs == nullptr);
}
template<typename T, typename S>
bool operator==(std::nullptr_t, const TwoWayPointer<T, S> & rhs)
{
    return rhs == nullptr;
}
template<typename T, typename S>
bool operator!=(std::nullptr_t, const TwoWayPointer<T, S> & rhs)
{
    return !(nullptr == rhs);
}
