#pragma once

#include <vector>
#include <numeric>
#include "util/view.hpp"
#include "debug/assert.hpp"
#include <cmath>
#include "math/my_math.hpp"

template<typename T>
struct Vector;

template<typename T>
struct VectorView : detail::BaseArrayView<T, VectorView<T>>
{
    using detail::BaseArrayView<T, VectorView<T>>::BaseArrayView;

    T dot(VectorView other) const
    {
        CHECK_FOR_PROGRAMMER_ERROR(this->size() == other.size());
        return std::inner_product(this->begin(), this->end(), other.begin(), T());
    }
    T length_squared() const
    {
        return dot(*this);
    }
    T length() const
    {
        return std::sqrt(length_squared());
    }
    T distance_squared(VectorView<const T> other) const
    {
        return std::inner_product(this->begin(), this->end(), other.begin(), T(), std::plus<>(), [](const T & lhs, const T & rhs){ return squared(lhs - rhs); });
    }
    T distance(VectorView<const T> other) const
    {
        return std::sqrt(distance_squared(other));
    }

    inline Vector<typename std::decay<T>::type> operator-() const;
};

template<typename T>
struct Vector : std::vector<T>
{
    Vector() = default;
    using std::vector<T>::vector;

    explicit Vector(VectorView<const T> values)
        : std::vector<T>(values.begin(), values.end())
    {
    }
    Vector & operator=(VectorView<const T> values)
    {
        this->assign(values.begin(), values.end());
        return *this;
    }

    T sum() const
    {
        return std::accumulate(this->begin(), this->end(), T());
    }
    Vector sum_to_one() const &
    {
        return Vector(*this).sum_to_one();
    }
    inline Vector sum_to_one() &&;
    T dot(VectorView<const T> other) const
    {
        return other.dot(*this);
    }
    T length_squared() const
    {
        return dot(*this);
    }
    T length() const
    {
        return std::sqrt(length_squared());
    }
    T distance_squared(VectorView<const T> other) const
    {
        return other.distance_squared(*this);
    }
    T distance(VectorView<const T> other) const
    {
        return other.distance(*this);
    }

    Vector & operator+=(T other)
    {
        for (T & e : *this)
            e += other;
        return *this;
    }
    Vector & operator-=(T other)
    {
        for (T & e : *this)
            e -= other;
        return *this;
    }
    Vector & operator*=(T other)
    {
        for (T & e : *this)
            e *= other;
        return *this;
    }
    Vector & operator/=(T other)
    {
        for (T & e : *this)
            e /= other;
        return *this;
    }
    Vector & operator+=(const Vector & other)
    {
        for (std::size_t i = 0, end = this->size(); i < end; ++i)
            (*this)[i] += other[i];
        return *this;
    }
    Vector & operator-=(const Vector & other)
    {
        for (std::size_t i = 0, end = this->size(); i < end; ++i)
            (*this)[i] -= other[i];
        return *this;
    }
    Vector & operator*=(const Vector & other)
    {
        for (std::size_t i = 0, end = this->size(); i < end; ++i)
            (*this)[i] *= other[i];
        return *this;
    }
    Vector & operator/=(const Vector & other)
    {
        for (std::size_t i = 0, end = this->size(); i < end; ++i)
            (*this)[i] /= other[i];
        return *this;
    }

    Vector operator-() const &
    {
        Vector copy(*this);
        for (T & elem : copy)
            elem = -elem;
        return copy;
    }
    Vector operator-() &&
    {
        Vector copy(std::move(*this));
        for (T & elem : copy)
            elem = -elem;
        return copy;
    }

    operator VectorView<T>()
    {
        return { this->data(), this->data() + this->size() };
    }
    operator VectorView<const T>() const
    {
        return { this->data(), this->data() + this->size() };
    }
};

template<typename T>
inline Vector<typename std::decay<T>::type> VectorView<T>::operator-() const
{
    return -Vector<typename std::decay<T>::type>(*this);
}

#define VECTOR_OPERATOR(op)\
template<typename T>\
Vector<T> operator op(Vector<T> && lhs, const Vector<T> & rhs)\
{\
    Vector<T> result(std::move(lhs));\
    for (std::size_t i = 0, end = result.size(); i < end; ++i)\
        result[i] = result[i] op rhs[i];\
    return result;\
}\
template<typename T>\
Vector<T> operator op(const Vector<T> & lhs, Vector<T> && rhs)\
{\
    Vector<T> result(std::move(rhs));\
    for (std::size_t i = 0, end = lhs.size(); i < end; ++i)\
        result[i] = lhs[i] op result[i];\
    return result;\
}\
template<typename T>\
Vector<T> operator op(Vector<T> && lhs, Vector<T> && rhs)\
{\
    return std::move(lhs) op rhs;\
}\
template<typename T>\
Vector<T> operator op(const Vector<T> & lhs, const Vector<T> & rhs)\
{\
    return Vector<T>(lhs) op rhs;\
}\
template<typename T>\
Vector<typename std::decay<T>::type> operator op(VectorView<T> lhs, VectorView<T> rhs)\
{\
    return Vector<typename std::decay<T>::type>(lhs) op rhs;\
}\
template<typename T>\
Vector<T> operator op(Vector<T> lhs, VectorView<const T> rhs)\
{\
    for (std::size_t i = 0, end = lhs.size(); i < end; ++i)\
        lhs[i] = lhs[i] op rhs[i];\
    return lhs;\
}\
template<typename T>\
Vector<T> operator op(VectorView<const T> lhs, Vector<T> rhs)\
{\
    for (std::size_t i = 0, end = lhs.size(); i < end; ++i)\
        rhs[i] = lhs[i] op rhs[i];\
    return rhs;\
}

VECTOR_OPERATOR(+)
VECTOR_OPERATOR(-)
VECTOR_OPERATOR(*)
#undef VECTOR_OPERATOR

#define VECTOR_SCALAR_OPERATOR(op)\
template<typename T>\
inline Vector<T> operator op(T lhs, Vector<T> rhs)\
{\
    for (T & f : rhs)\
    {\
        f = lhs op f;\
    }\
    return rhs;\
}\
template<typename T>\
inline Vector<T> operator op(Vector<T> lhs, T rhs)\
{\
    for (T & f : lhs)\
    {\
        f = f op rhs;\
    }\
    return lhs;\
}\
template<typename T>\
inline Vector<T> operator op(VectorView<const T> lhs, T rhs)\
{\
    return Vector<T>(lhs) op rhs;\
}\
template<typename T>\
inline Vector<T> operator op(T lhs, VectorView<const T> rhs)\
{\
    return lhs op Vector<T>(rhs);\
}
VECTOR_SCALAR_OPERATOR(+)
VECTOR_SCALAR_OPERATOR(-)
VECTOR_SCALAR_OPERATOR(*)
VECTOR_SCALAR_OPERATOR(/)
#undef VECTOR_SCALAR_OPERATOR
template<typename T>
inline Vector<T> Vector<T>::sum_to_one() &&
{
    T s = sum();
    return std::move(*this) / s;
}

#define VECTOR_STD_MATH_FUNCTION(func)\
template<typename T>\
inline Vector<T> func(Vector<T> vec)\
{\
    for (T & e : vec)\
        e = std::func(e);\
    return vec;\
}\
template<typename T>\
inline Vector<T> func(VectorView<const T> vec)\
{\
    return func(Vector<T>(vec));\
}
VECTOR_STD_MATH_FUNCTION(abs)
VECTOR_STD_MATH_FUNCTION(exp)
VECTOR_STD_MATH_FUNCTION(log)
VECTOR_STD_MATH_FUNCTION(tanh)
#undef VECTOR_MATH_FUNCTION
