#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <cstddef>
#include "util/function.hpp"
#include "util/two_way_pointer.hpp"
#include "util/type_erasure.hpp"

namespace sig2
{
template<typename...>
class Signal;
template<typename... Args>
class Slot;
template<typename... Args>
class Connection;

namespace detail
{
template<typename Func>
using MovableFunction = CallableTypeErasure<Func, sizeof(void *) * 2, BaseTypeErasure, MoveVTable>;
template<typename Func>
using SlotFunction = MovableFunction<Func>;
template<typename... Args>
using SlotPointerFunc = SlotFunction<void (Slot<Args...> &, Args...)>;
}
template<typename... Args>
class Connection
{
public:
    Connection() = default;
    explicit Connection(Slot<Args...> & slot)
        : slot(&slot.connection)
    {
    }
    Connection(const Connection &) = delete;
    Connection(Connection && other) = default;
    Connection & operator=(const Connection &) = delete;
    Connection & operator=(Connection && other)
    {
        disconnect();
        slot = std::move(other.slot);
        return *this;
    }

    ~Connection()
    {
        disconnect();
    }

    void disconnect()
    {
        if (slot)
            slot.cut_connection();
    }

protected:
    template<typename...>
    friend class Signal;

    TwoWayPointer<void, detail::SlotPointerFunc<Args...>> slot;
};

template<typename... Args>
class Slot
{
    TwoWayPointer<detail::SlotPointerFunc<Args...>, void> connection;

    template<typename...>
    friend class Connection;
    template<typename, typename, typename...>
    friend class ConnectionWithResult;

public:
    template<typename Func>
    Slot(Func && func)
        : connection(std::forward<Func>(func))
    {
    }

    void operator()(const Args &... args)
    {
        connection.value(*this, args...);
    }

    bool is_empty() const
    {
        return !connection;
    }
    void clear()
    {
        connection.cut_connection();
    }
};

enum class ConnectionFilter
{
    Call,
    Skip,
    Disconnect,
    CallThenDisconnect
};

namespace detail
{
// my own version of std::result_of used in
// ConnectionWithResult::map below because I
// need a specialization for the case where
// the old function returns void, which wouldn't
//  be a valid function call
template<typename Result, typename... Args>
struct ResultOf : std::result_of<Result (Args...)>
{
};
template<typename Result>
struct ResultOf<Result, void> : std::result_of<Result ()>
{
};

template<typename T, typename Result, typename SlotType, typename... Args>
struct SlotCaller
{
    SlotCaller(T func)
        : func(to_functor(std::move(func)))
    {
    }

    typename functor_type<T>::type func;

    Result operator()(SlotType &, const Args &... args)
    {
        return func(args...);
    }
    template<typename Next>
    void call_with_next(SlotType & slot, Next && next, const Args &... args)
    {
        next(slot, func(args...));
    }
};
template<typename T, typename SlotType, typename... Args>
struct SlotCaller<T, void, SlotType, Args...>
{
    SlotCaller(T func)
        : func(to_functor(std::move(func)))
    {
    }

    typename functor_type<T>::type func;

    void operator()(SlotType &, const Args &... args)
    {
        func(args...);
    }
    template<typename Next>
    void call_with_next(SlotType & slot, Next && next, const Args &... args)
    {
        func(args...);
        next(slot);
    }
};
template<typename T, typename Result, typename SlotType>
struct SlotCaller<T, Result, SlotType, void>
{
    SlotCaller(T func)
        : func(to_functor(std::move(func)))
    {
    }

    typename functor_type<T>::type func;
    Result operator()(SlotType &)
    {
        return func();
    }
    template<typename Next>
    void call_with_next(SlotType & slot, Next && next)
    {
        next(slot, func());
    }
};
template<typename T, typename SlotType>
struct SlotCaller<T, void, SlotType, void>
{
    SlotCaller(T func)
        : func(to_functor(std::move(func)))
    {
    }

    typename functor_type<T>::type func;
    void operator()(SlotType &)
    {
        func();
    }
    template<typename Next>
    void call_with_next(SlotType & slot, Next && next)
    {
        func();
        next(slot);
    }
};

template<typename OldResult, typename OldFunc, typename Func>
struct Mapped : OldFunc, Func
{
    Mapped(OldFunc && target, Func && func)
        : OldFunc(std::move(target)), Func(std::move(func))
    {
    }
    template<typename SlotType, typename... Args>
    void operator()(SlotType & slot, Args &&... args)
    {
        static_cast<OldFunc &>(*this).call_with_next(slot, static_cast<Func &>(*this), std::forward<Args>(args)...);
    }

    template<typename SlotType, typename T, typename... Args>
    void call_with_next(SlotType & slot, T && next, Args &&... args)
    {
        static_cast<OldFunc &>(*this).call_with_next(slot, [&](SlotType & slot, OldResult result)
        {
            static_cast<Func &>(*this).call_with_next(slot, std::forward<T>(next), std::forward<OldResult>(result));
        }, std::forward<Args>(args)...);
    }
};
template<typename OldFunc, typename Func>
struct Mapped<void, OldFunc, Func> : OldFunc, Func
{
    Mapped(OldFunc && target, Func && func)
        : OldFunc(std::move(target)), Func(std::move(func))
    {
    }
    template<typename SlotType, typename... Args>
    void operator()(SlotType & slot, Args &&... args)
    {
        static_cast<OldFunc &>(*this).call_with_next(slot, static_cast<Func &>(*this), std::forward<Args>(args)...);
    }
    template<typename SlotType, typename T, typename... Args>
    void call_with_next(SlotType & slot, T && next, Args &&... args)
    {
        static_cast<OldFunc &>(*this).call_with_next(slot, [&](SlotType & slot)
        {
            static_cast<Func &>(*this).call_with_next(slot, std::forward<T>(next));
        }, std::forward<Args>(args)...);
    }
};

template<typename OldResult, typename Target, typename Func>
struct Filtered : Target, Func
{
    Filtered(Target && target, Func && func)
        : Target(std::move(target)), Func(std::move(func))
    {
    }
    template<typename SlotType, typename... Args>
    void operator()(SlotType & slot, Args &&... args)
    {
        static_cast<Target &>(*this).call_with_next(slot, [&](SlotType & slot, const OldResult & result)
        {
            switch(static_cast<Func &>(*this)(slot, result))
            {
            case ConnectionFilter::Call:
            case ConnectionFilter::Skip:
                break;
            case ConnectionFilter::Disconnect:
            case ConnectionFilter::CallThenDisconnect:
                slot.clear();
                break;
            }
        }, std::forward<Args>(args)...);
    }
    template<typename SlotType, typename T, typename... Args>
    void call_with_next(SlotType & slot, T && next, Args &&... args)
    {
        static_cast<Target &>(*this).call_with_next(slot, [&](SlotType & slot, OldResult result)
        {
            switch(static_cast<Func &>(*this)(slot, static_cast<const OldResult &>(result)))
            {
            case ConnectionFilter::Call:
                next(slot, std::forward<OldResult>(result));
                break;
            case ConnectionFilter::CallThenDisconnect:
            {
                next(slot, std::forward<OldResult>(result));
                slot.clear();
                break;
            }
            case ConnectionFilter::Skip:
                break;
            case ConnectionFilter::Disconnect:
                slot.clear();
                break;
            }
        }, std::forward<Args>(args)...);
    }
};
template<typename Target, typename Func>
struct Filtered<void, Target, Func> : Target, Func
{
    Filtered(Target && target, Func && func)
        : Target(std::move(target)), Func(std::move(func))
    {
    }
    template<typename SlotType, typename... Args>
    void operator()(SlotType & slot, Args &&... args)
    {
        static_cast<Target &>(*this)(slot, std::forward<Args>(args)...);
        switch(static_cast<Func &>(*this)(slot))
        {
        case ConnectionFilter::Call:
        case ConnectionFilter::Skip:
            break;
        case ConnectionFilter::Disconnect:
        case ConnectionFilter::CallThenDisconnect:
            slot.clear();
            break;
        }
    }
    template<typename SlotType, typename T, typename... Args>
    void call_with_next(SlotType & slot, T && next, Args &&... args)
    {
        static_cast<Target &>(*this)(slot, std::forward<Args>(args)...);
        switch(static_cast<Func &>(*this)(slot))
        {
        case ConnectionFilter::Call:
            next(slot);
            break;
        case ConnectionFilter::CallThenDisconnect:
        {
            next(slot);
            slot.clear();
            break;
        }
        case ConnectionFilter::Skip:
            break;
        case ConnectionFilter::Disconnect:
            slot.clear();
            break;
        }
    }
};
}

template<typename Result, typename Target, typename... Args>
class ConnectionWithResult : public Connection<Args...>
{
public:
    using Connection<Args...>::Connection;

    template<typename Func, typename NewResult = typename detail::ResultOf<Func, Result>::type, typename StoredFunc = detail::SlotCaller<Func, NewResult, Slot<Args...>, Result>, typename NewFunc = typename detail::Mapped<Result, Target, StoredFunc>>
    ConnectionWithResult<NewResult, NewFunc, Args...> map(Func func)
    {
        *this->slot = NewFunc{std::move(*this->slot->template target<Target>()), StoredFunc(std::move(func))};
        return reinterpret_cast<ConnectionWithResult<NewResult, NewFunc, Args...> &&>(*this);
    }
    template<typename Func, typename StoredFunc = detail::SlotCaller<Func, ConnectionFilter, Slot<Args...>, Result>, typename NewFunc = typename detail::Filtered<Result, Target, StoredFunc>>
    ConnectionWithResult<Result, NewFunc, Args...> filter(Func func)
    {
        *this->slot = NewFunc{std::move(*this->slot->template target<Target>()), StoredFunc(std::move(func))};
        return reinterpret_cast<ConnectionWithResult<Result, NewFunc, Args...> &&>(*this);
    }
};

namespace detail
{
struct TakeFilter
{
    int n;
    template<typename... Args>
    ConnectionFilter operator()(const Args &...)
    {
        if (--n <= 0)
            return ConnectionFilter::Disconnect;
        else
            return ConnectionFilter::Call;
    }
};
}

template<typename Connection>
auto take_n(Connection & connection, int n)
{
    return connection.filter(detail::TakeFilter{n});
}
template<typename Connection>
auto take_n(Connection && connection, int n)
{
    return take_n(connection, n);
}

namespace detail
{
struct UntilEnd
{
    TwoWayPointer<void, void> marker;
    template<typename... Args>
    void operator()(const Args &...)
    {
        marker.cut_connection();
    }
};
}

template<typename Connection, typename Connection2>
auto until(Connection & connection, Connection2 && end)
{
    TwoWayPointer<void, void> a_connection;
    TwoWayPointer<void, void> b_connection(&a_connection);
    auto end_connection = end.map(detail::UntilEnd{std::move(a_connection)});
    return connection.filter([end = std::move(end_connection), marker = std::move(b_connection)](const auto &...)
    {
        if (marker)
            return ConnectionFilter::Call;
        else
            return ConnectionFilter::Disconnect;
    });
}
template<typename Connection, typename Connection2>
auto until(Connection && connection, Connection2 && end)
{
    return until(connection, std::move(end));
}

namespace detail
{
template<typename... Args>
struct TupleOfImpl
{
    typedef std::tuple<Args...> type;
};
template<typename Arg>
struct TupleOfImpl<Arg>
{
    typedef Arg type;
};
template<>
struct TupleOfImpl<>
{
    typedef void type;
};

template<typename... Args>
using TupleOf = typename TupleOfImpl<Args...>::type;
struct ToTuple
{
    template<typename... Args>
    std::tuple<Args...> operator()(Args ... args)
    {
        return std::make_tuple<Args...>(std::forward<Args>(args)...);
    }
    template<typename Arg>
    Arg operator()(Arg arg)
    {
        return arg;
    }
    void operator()()
    {
    }
};
}

template<typename... Args>
class Signal
{
public:
    Signal() = default;
    Signal(Signal && other)
        : to_add(std::move(other.to_add))
        , slots(std::move(other.slots))
    {
    }
    Signal & operator=(Signal && other)
    {
        slots = std::move(other.slots);
        to_add = std::move(other.to_add);
        return *this;
    }

    void emit(const Args &... args)
    {
        perform_delayed_adds();
        auto end = slots.end();
        slots.erase(std::remove_if(slots.begin(), end, [&](Slot<Args...> & slot) -> bool
        {
            if (slot.is_empty())
                return true;
            slot(args...);
            return slot.is_empty();
        }), end);
    }
    /**
     * Connect the given function or functor to this signal. It will be called
     * every time that someone emits this signal.
     * This function returns a SignalDisconnecter that will disconnect the slot
     * when it is destroyed. Therefore if you ignore the return value this
     * function will have no effect because the Disconnecter will disconnect
     * immediately
     */
    template<typename T, typename Result = typename std::result_of<T(Args...)>::type>
    ConnectionWithResult<Result, detail::SlotCaller<T, Result, Slot<Args...>, Args...>, Args...> map(T slot)
    {
        typedef detail::SlotCaller<T, Result, Slot<Args...>, Args...> FunctorType;
        Slot<Args...> new_slot(FunctorType{std::move(slot)});
        ConnectionWithResult<Result, FunctorType, Args...> to_return(new_slot);
        if (slots.size() == slots.capacity())
        {
            if (to_add.capacity() == to_add.size())
                to_add.erase(std::remove_if(to_add.begin(), to_add.end(), std::mem_fn(&Slot<Args...>::is_empty)), to_add.end());
            to_add.emplace_back(std::move(new_slot));
        }
        else
            slots.emplace_back(std::move(new_slot));
        return to_return;
    }

    template<typename T>
    auto filter(T filter)
    {
        return map(detail::ToTuple{}).filter(std::move(filter));
    }

    void perform_delayed_adds()
    {
        for (auto & add : to_add)
        {
            slots.push_back(std::move(add));
        }
        to_add.clear();
    }
    void clear()
    {
        to_add.clear();
        slots.clear();
    }
    size_t size() const
    {
        return slots.size();
    }
    size_t capacity() const
    {
        return slots.capacity();
    }
    void reserve(size_t count)
    {
        slots.reserve(count);
    }

private:
    template<typename...>
    friend class Connection;

    std::vector<Slot<Args...>> slots;
    // containers for storing objects that should be added on the next emit
    // this is needed because slots have a tendency to add new things in
    // their callbacks, and the slots vector could reallocate
    std::vector<Slot<Args...>> to_add;
};
}
