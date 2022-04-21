#pragma once
#include <utility>
#include <functional>

namespace detail
{
template<typename>
struct MemFn;
template<typename Result, typename Class, typename... Arguments>
struct MemFn<Result (Class::*)(Arguments...)>
{
    Result (Class::*func)(Arguments...);
    Result operator()(Class & object, Arguments... args) const
    {
        return (object.*func)(std::forward<Arguments>(args)...);
    }
};
template<typename Result, typename Class, typename... Arguments>
struct MemFn<Result (Class::*)(Arguments...) const>
{
    Result (Class::*func)(Arguments...) const;
    Result operator()(Class & object, Arguments... args) const
    {
        return (object.*func)(std::forward<Arguments>(args)...);
    }
    Result operator()(const Class & object, Arguments... args) const
    {
        return (object.*func)(std::forward<Arguments>(args)...);
    }
};
template<typename Result, typename Class>
struct MemFn<Result (Class::*)>
{
    Result Class::*ptr;
    Result & operator()(Class & object) const
    {
        return object.*ptr;
    }
    const Result & operator()(const Class & object) const
    {
        return object.*ptr;
    }
};
template<typename>
struct PtrFn;
template<typename Result, typename... Arguments>
struct PtrFn<Result (*)(Arguments...)>
{
    Result (*func)(Arguments...);
    Result operator()(Arguments... args) const
    {
        return func(std::forward<Arguments>(args)...);
    }

    bool operator==(const PtrFn & other) const
    {
        return func == other.func;
    }
    bool operator!=(const PtrFn & other) const
    {
        return !(*this == other);
    }
};
}

template<typename T>
T to_functor(T func)
{
    return func;
}
template<typename Result, typename Class, typename... Arguments>
detail::MemFn<Result (Class::*)(Arguments...)> to_functor(Result (Class::*func)(Arguments...))
{
    return {func};
}
template<typename Result, typename Class, typename... Arguments>
detail::MemFn<Result (Class::*)(Arguments...) const> to_functor(Result (Class::*func)(Arguments...) const)
{
    return {func};
}
template<typename Result, typename Class>
detail::MemFn<Result (Class::*)> to_functor(Result (Class::*ptr))
{
    return {ptr};
}
template<typename Result, typename... Arguments>
detail::PtrFn<Result (*)(Arguments...)> to_functor(Result (*func)(Arguments...))
{
    return {func};
}

template<typename T>
struct functor_type
{
    using type = decltype(to_functor(std::declval<T>()));
};
