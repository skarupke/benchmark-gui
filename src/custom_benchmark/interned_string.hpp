#pragma once

#include <string>
#include <string_view>
#include <iosfwd>

struct interned_string
{
    interned_string() = default;
    explicit interned_string(std::string_view str);
    template<size_t Size>
    interned_string(const char (&array)[Size])
        : interned_string(std::string_view(array))
    {
    }

    operator std::string_view() const
    {
        return str;
    }
    std::string_view view() const
    {
        return str;
    }
    const char * data() const
    {
        return str.data();
    }
    size_t size() const
    {
        return str.size();
    }

    friend bool operator==(const interned_string & l, const interned_string & r)
    {
        return l.str.data() == r.str.data();
    }
    friend bool operator!=(const interned_string & l, const interned_string & r)
    {
        return !(l == r);
    }
    friend bool operator==(const interned_string & l, std::string_view r)
    {
        return l.str == r;
    }
    friend bool operator!=(const interned_string & l, std::string_view r)
    {
        return !(l == r);
    }
    friend bool operator==(std::string_view l, const interned_string & r)
    {
        return l == r.str;
    }
    friend bool operator!=(std::string_view l, const interned_string & r)
    {
        return !(l == r);
    }
    template<size_t Size>
    friend bool operator==(const interned_string & l, const char(&r)[Size])
    {
        return l == std::string_view(r);
    }
    template<size_t Size>
    friend bool operator!=(const interned_string & l, const char(&r)[Size])
    {
        return !(l == r);
    }
    template<size_t Size>
    friend bool operator==(const char(&l)[Size], const interned_string & r)
    {
        return std::string_view(l) == r.str;
    }
    template<size_t Size>
    friend bool operator!=(const char(&l)[Size], const interned_string & r)
    {
        return !(l == r);
    }

    friend interned_string operator+(const interned_string & l, std::string_view r)
    {
        std::string sum;
        sum.reserve(l.str.size() + r.size());
        sum += l.str;
        sum += r;
        return interned_string(sum);
    }
    friend interned_string operator+(std::string_view l, const interned_string & r)
    {
        std::string sum;
        sum.reserve(l.size() + r.str.size());
        sum += l;
        sum += r.str;
        return interned_string(sum);
    }

    friend std::ostream & operator<<(std::ostream & l, const interned_string & r)
    {
        return l << r.str;
    }

    struct pointer_less
    {
        bool operator()(const interned_string & l, const interned_string & r) const
        {
            return l.str.data() < r.str.data();
        }
    };
    struct string_less
    {
        bool operator()(const interned_string & l, const interned_string & r) const
        {
            return l.str < r.str;
        }
    };

private:
    std::string_view str;
};

template<typename T>
interned_string to_interned_string(const T & value)
{
    return interned_string(std::to_string(value));
}

namespace std
{
template<>
struct hash<interned_string>
{
    size_t operator()(const interned_string & str) const
    {
        return reinterpret_cast<size_t>(str.view().data());
    }
};
}

