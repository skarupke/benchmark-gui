#pragma once

#include <functional>
#include <type_traits>
#include <tuple>

template<typename... Callable>
class OverloadedCallWrapper
{
    template<typename T>
    struct PointerOrReference
    {
        using type = T &;
    };
    template<typename R, typename... Args>
    struct PointerOrReference<R (*)(Args...)>
    {
        using type = R(*)(Args...);
    };
    template<typename R, typename... Args>
    struct PointerOrReference<R (* const)(Args...)>
    {
        using type = R(* const)(Args...);
    };
    template<typename T, typename C>
    struct PointerOrReference<T C::*>
    {
        using type = T C::*;
    };
    template<typename T, typename C>
    struct PointerOrReference<T C::* const>
    {
        using type = T C::* const;
    };
    template<typename T>
    struct PointerOrReference<std::reference_wrapper<T>>
    {
        using type = std::reference_wrapper<T>;
    };
    template<typename T>
    struct PointerOrReference<T &>
    {
        using type = typename PointerOrReference<T>::type;
    };
    template<typename T>
    struct PointerOrReference<T &&>
    {
        using type = typename PointerOrReference<T>::type;
    };
    using TupleType = std::tuple<typename PointerOrReference<Callable>::type...>;

    template<size_t Index, typename Arg>
    decltype(auto) TryInOrder(Arg && arg)
    {
        if constexpr (std::is_invocable_v<std::tuple_element_t<Index, TupleType>, decltype(arg)>)
        {
            return std::invoke(std::get<Index>(callables), std::forward<Arg>(arg));
        }
        else
        {
            return TryInOrder<Index + 1>(std::forward<Arg>(arg));
        }
    }
    template<size_t Index, typename Arg>
    decltype(auto) TryInOrder(Arg && arg) const
    {
        if constexpr (std::is_invocable_v<std::tuple_element_t<Index, TupleType>, decltype(arg)>)
        {
            return std::invoke(std::get<Index>(callables), std::forward<Arg>(arg));
        }
        else
        {
            return TryInOrder<Index + 1>(std::forward<Arg>(arg));
        }
    }

    template<size_t Index, typename T>
    struct IsCallableInternal
    {
        static constexpr bool value = std::is_invocable_v<std::tuple_element_t<Index, TupleType>, T>
                    || IsCallableInternal<Index + 1, T>::value;
    };
    template<typename T>
    struct IsCallableInternal<std::tuple_size_v<TupleType>, T>
    {
        static constexpr bool value = false;
    };

public:
    TupleType callables;
    template<typename Arg>
    decltype(auto) operator()(Arg && arg)
    {
        return TryInOrder<0>(std::forward<Arg>(arg));
    }
    template<typename Arg>
    decltype(auto) operator()(Arg && arg) const
    {
        return TryInOrder<0>(std::forward<Arg>(arg));
    }

    template<typename T>
    static constexpr bool IsCallable = IsCallableInternal<0, T>::value;
};

template<typename Callable>
class CallableWrapper
{
public:
    std::remove_reference_t<Callable> callable;
    template<typename Arg>
    decltype(auto) operator()(Arg && arg)
    {
        return std::invoke(callable, std::forward<Arg>(arg));
    }
    template<typename Arg>
    decltype(auto) operator()(Arg && arg) const
    {
        return std::invoke(callable, std::forward<Arg>(arg));
    }

    template<typename T>
    static constexpr bool IsCallable = std::is_invocable_v<Callable, T>;
};
