#pragma once

#include <utility>

template<typename T>
struct ScopeExit
{
    ScopeExit(T && value)
        : value(std::move(value))
    {
    }
    ScopeExit(ScopeExit && other)
        : value(std::move(other.value))
        , should_run(other.should_run)
    {
        other.should_run = false;
    }
    ~ScopeExit()
    {
        if (should_run)
            value();
    }

    void cancel()
    {
        should_run = false;
    }

private:
    T value;
    bool should_run = true;
};


template<typename T>
ScopeExit<T> AtScopeExit(T value)
{
    return {std::move(value)};
}
