#pragma once

#include <cstdint>
#include <memory>
#include <debug/assert.hpp>
#include <algorithm>
#include <iostream>

namespace detail
{
struct VoidUniquePointer
{
    VoidUniquePointer() = default;
    VoidUniquePointer(VoidUniquePointer && other)
        : ptr(other.ptr)
    {
        other.ptr = nullptr;
    }
    VoidUniquePointer & operator=(VoidUniquePointer && other)
    {
        swap(other);
        return *this;
    }
    ~VoidUniquePointer()
    {
        CHECK_FOR_PROGRAMMER_ERROR(!ptr);
    }

    void swap(VoidUniquePointer & other)
    {
        std::swap(ptr, other.ptr);
    }

    void reset(void * new_ptr)
    {
       CHECK_FOR_PROGRAMMER_ERROR(!ptr);
       ptr = new_ptr;
    }

    void * get() const
    {
        return ptr;
    }

    void * release()
    {
        void * result = ptr;
        ptr = nullptr;
        return result;
    }

private:
    void * ptr = nullptr;
};
}

struct FingerTreeCountMeasure
{
    using type = int;

    static int empty()
    {
        return 0;
    }

    template<typename T>
    static int measure(const T &)
    {
        return 1;
    }
    static type combine(int l, int r)
    {
        return l + r;
    }
};

template<typename T, typename Measure = FingerTreeCountMeasure>
struct FingerTree
{
private:
    struct SingleInUnion { T value; };

    static void print_indentation(std::ostream & out, int indentation)
    {
        for (int i = 0; i < indentation; ++i)
            out << ' ';
    }

    struct Tuple
    {
        enum Type
        {
            LastLayer,
            IntermediateLayer,
        };
        Type type : 1;
        size_t size : 15;
        size_t capacity : 16;

        typename Measure::type measure;

        detail::VoidUniquePointer data;

        Tuple()
            : type(LastLayer)
            , size(0), capacity(0)
            , measure(Measure::empty())
        {
        }

        Tuple(Type type, size_t capacity)
            : type(type)
            , size(0)
            , capacity(capacity)
            , measure(Measure::empty())
        {
            if (type == IntermediateLayer)
                data.reset(new Tuple[capacity]());
            else
                data.reset(new SingleInUnion[capacity]());
        }
        Tuple(Tuple && other)
            : type(other.type)
            , size(other.size)
            , capacity(other.capacity)
            , measure(std::move(other.measure))
            , data(std::move(other.data))
        {
            other.size = other.capacity = 0;
            other.measure = Measure::empty();
        }

        Tuple & operator=(Tuple && other)
        {
            using std::swap;
            Type tmp_type = type;
            type = other.type;
            other.type = tmp_type;

            size_t tmp_size = size;
            size = other.size;
            other.size = tmp_size;

            size_t tmp_capacity = capacity;
            capacity = other.capacity;
            other.capacity = tmp_capacity;

            swap(measure, other.measure);

            data.swap(other.data);
            return *this;
        }

        ~Tuple()
        {
            if (type == LastLayer)
            {
                SingleInUnion * elems = static_cast<SingleInUnion *>(data.get());
                for (size_t i = 0, end = size; i < end; ++i)
                {
                    elems[i].value.~T();
                }
                delete[] elems;
            }
            else
            {
                delete[] static_cast<Tuple *>(data.get());
            }
            data.release();
        }

        void print(std::ostream & out, int indentation) const
        {
            if (type == IntermediateLayer)
            {
                const Tuple * lower_layer = static_cast<const Tuple *>(data.get());
                for (size_t i = 0, end = size; i < end; ++i)
                    lower_layer[i].print(out, indentation/* + 2*/);
            }
            else
            {
                const SingleInUnion * items = static_cast<SingleInUnion *>(data.get());
                for (size_t i = 0, end = size; i < end; ++i)
                {
                    print_indentation(out, indentation);
                    out << items[i].value << '\n';
                }
            }
        }

        template<typename F>
        bool iter(F && f) const
        {
            if (type == IntermediateLayer)
            {
                const Tuple * lower_layer = static_cast<const Tuple *>(data.get());
                for (size_t i = 0, end = size; i < end; ++i)
                {
                    if (!lower_layer[i].iter(f))
                        return false;
                }
            }
            else
            {
                const SingleInUnion * items = static_cast<SingleInUnion *>(data.get());
                for (size_t i = 0, end = size; i < end; ++i)
                {
                    if (!f(items[i].value))
                        return false;
                }
            }
            return true;
        }
        template<typename F>
        bool iter_left(F && f) const
        {
            if (type == IntermediateLayer)
            {
                const Tuple * lower_layer = static_cast<const Tuple *>(data.get());
                for (size_t i = size; i > 0;)
                {
                    --i;
                    if (!lower_layer[i].iter(f))
                        return false;
                }
            }
            else
            {
                const SingleInUnion * items = static_cast<SingleInUnion *>(data.get());
                for (size_t i = size; i > 0;)
                {
                    --i;
                    if (!f(items[i].value))
                        return false;
                }
            }
            return true;
        }
        template<typename Check, typename F>
        bool iter_right_from(Check & check, F & f) const
        {
            size_t i = 0;
            size_t end = size;
            if (type == IntermediateLayer)
            {
                const Tuple * lower_layer = static_cast<const Tuple *>(data.get());
                for (; i < end; ++i)
                {
                    if (!check(lower_layer[i].measure))
                        continue;
                    if (!lower_layer[i].iter_right_from(check, f))
                        return false;
                    break;
                }
                for (++i; i < end; ++i)
                {
                    if (!lower_layer[i].iter(f))
                        return false;
                }
                return true;
            }
            else
            {
                const SingleInUnion * items = static_cast<SingleInUnion *>(data.get());
                for (; i < end; ++i)
                {
                    if (!check(Measure::measure(items[i].value)))
                        continue;
                    if (!f(items[i].value))
                        return false;
                    break;
                }
                for (++i; i < end; ++i)
                {
                    if (!f(items[i].value))
                        return false;
                }
                return true;
            }
            return true;
        }
        template<typename Check, typename F>
        bool iter_right_from_left(Check && check, F && f) const
        {
            size_t i = size;
            if (type == IntermediateLayer)
            {
                const Tuple * lower_layer = static_cast<const Tuple *>(data.get());
                while (i > 0)
                {
                    --i;
                    if (!check(lower_layer[i].measure))
                        continue;
                    if (!lower_layer[i].iter_right_from(check, f))
                        return false;
                    break;
                }
                while (i > 0)
                {
                    --i;
                    if (!lower_layer[i].iter(f))
                        return false;
                }
                return true;
            }
            else
            {
                const SingleInUnion * items = static_cast<SingleInUnion *>(data.get());
                while (i > 0)
                {
                    --i;
                    if (!check(Measure::measure(items[i].value)))
                        continue;
                    if (!f(items[i].value))
                        return false;
                    break;
                }
                while (i > 0)
                {
                    --i;
                    if (!f(items[i].value))
                        return false;
                }
            }
            return true;
        }

        void push_back_left(const T & new_left)
        {
            measure = Measure::combine(Measure::measure(new_left), std::move(measure));
            push_back(new_left);
        }
        void push_back_left(T && new_left)
        {
            measure = Measure::combine(Measure::measure(new_left), std::move(measure));
            push_back(std::move(new_left));
        }
        void push_back_left(Tuple && val)
        {
            measure = Measure::combine(val.measure, std::move(measure));
            push_back(std::move(val));
        }
        void push_back_right(const T & new_right)
        {
            measure = Measure::combine(std::move(measure), Measure::measure(new_right));
            push_back(new_right);
        }
        void push_back_right(T && new_right)
        {
            measure = Measure::combine(std::move(measure), Measure::measure(new_right));
            push_back(std::move(new_right));
        }
        void push_back_right(Tuple && val)
        {
            measure = Measure::combine(std::move(measure), val.measure);
            push_back(std::move(val));
        }

        void pop_back_left()
        {
            pop_back();
            recalculate_measure_left();
        }
        void pop_back_right()
        {
            pop_back();
            recalculate_measure_right();
        }
        void pop_front_left()
        {
            pop_front();
            recalculate_measure_left();
        }
        void pop_front_right()
        {
            pop_front();
            recalculate_measure_right();
        }

    private:

        void recalculate_measure_left()
        {
            measure = Measure::empty();
            if (type == LastLayer)
            {
                T * items = static_cast<T *>(data.get());
                for (size_t i = 0, end = size; i < end; ++i)
                    measure = Measure::combine(Measure::measure(items[i]), std::move(measure));
            }
            else
            {
                Tuple * items = static_cast<Tuple *>(data.get());
                for (size_t i = 0, end = size; i < end; ++i)
                    measure = Measure::combine(items[i].measure, std::move(measure));
            }
        }

        void recalculate_measure_right()
        {
            measure = Measure::empty();
            if (type == LastLayer)
            {
                T * items = static_cast<T *>(data.get());
                for (size_t i = 0, end = size; i < end; ++i)
                    measure = Measure::combine(std::move(measure), Measure::measure(items[i]));
            }
            else
            {
                Tuple * items = static_cast<Tuple *>(data.get());
                for (size_t i = 0, end = size; i < end; ++i)
                    measure = Measure::combine(std::move(measure), items[i].measure);
            }
        }

        void push_back(const T & val)
        {
            CHECK_FOR_PROGRAMMER_ERROR(type == LastLayer);
            CHECK_FOR_PROGRAMMER_ERROR(size < capacity);
            SingleInUnion * items = static_cast<SingleInUnion *>(data.get());
            new (&items[size].value) T(val);
            ++size;
        }
        void push_back(T && val)
        {
            CHECK_FOR_PROGRAMMER_ERROR(type == LastLayer);
            CHECK_FOR_PROGRAMMER_ERROR(size < capacity);
            SingleInUnion * items = static_cast<SingleInUnion *>(data.get());
            new (&items[size].value) T(std::move(val));
            ++size;
        }
        void push_back(Tuple && val)
        {
            CHECK_FOR_PROGRAMMER_ERROR(type == IntermediateLayer);
            CHECK_FOR_PROGRAMMER_ERROR(size < capacity);
            Tuple * items = static_cast<Tuple *>(data.get());
            items[size] = std::move(val);
            ++size;
        }

        void pop_back()
        {
            --size;
            if (type == LastLayer)
            {
                T * items = static_cast<T *>(data.get());
                items[size].~T();
            }
            else
            {
                Tuple * items = static_cast<Tuple *>(data.get());
                items[size] = Tuple();
            }
        }
        void pop_front()
        {
            if (type == Tuple::LastLayer)
            {
                T * elems = static_cast<T *>(data.get());
                std::move(elems + 1, elems + size, elems);
                --size;
                elems[size].~T();
            }
            else
            {
                Tuple * elems = static_cast<Tuple *>(data.get());
                std::move(elems + 1, elems + size, elems);
                --size;
                elems[size] = Tuple();
            }
        }
    };

    struct Seq
    {
        Tuple left;
        Tuple right;
        std::unique_ptr<Seq> middle;
        typename Measure::type measure;
        static constexpr size_t SideCapacity = 3;

        Seq()
            : left(Tuple::LastLayer, SideCapacity)
            , right(Tuple::LastLayer, SideCapacity)
            , measure(Measure::empty())
        {
        }
        Seq(Tuple && single)
            : left(Tuple::IntermediateLayer, SideCapacity)
            , right(Tuple::IntermediateLayer, SideCapacity)
            , measure(single.measure)
        {
            left.push_back_left(std::move(single));
        }

        void print_tree(std::ostream & out, int indentation) const
        {
            left.print(out, indentation);
            if (middle)
                middle->print_tree(out, indentation + 2);
            else
            {
                print_indentation(out, indentation);
                out << "None\n";
            }
            right.print(out, indentation);
        }

        template<typename F>
        bool iter(F && f) const
        {
            if (left.size == 0)
                return true;
            if (!left.iter_left(f))
                return false;
            if (right.size == 0)
                return true;
            if (middle && !middle->iter(f))
                return false;
            return right.iter(f);
        }
        template<typename Check, typename F>
        bool iter_right_from(Check & check, F & f)
        {
            if (check(left.measure))
            {
                if (!left.iter_right_from_left(check, f))
                    return false;
                if (middle && !middle->iter(f))
                    return false;
                return right.iter(f);
            }
            else if (middle && check(middle->measure))
            {
                if (!middle->iter_right_from(check, f))
                    return false;
                return right.iter(f);
            }
            else if (check(right.measure))
            {
                return right.iter_right_from(check, f);
            }
            return true;
        }

        template<typename U>
        void make_room_left()
        {
            constexpr typename Tuple::Type layer_type = std::is_same<U, T>::value ? Tuple::LastLayer : Tuple::IntermediateLayer;
            Tuple one_layer_down(layer_type, 2);
            U * left_elems = static_cast<U *>(left.data.get());
            one_layer_down.push_back_right(std::move(left_elems[1]));
            one_layer_down.push_back_right(std::move(left_elems[0]));
            std::move(left_elems + 2, left_elems + SideCapacity, left_elems);
            left.size -= 2;
            if (middle)
                middle->push_left(std::move(one_layer_down));
            else
                middle.reset(new Seq(std::move(one_layer_down)));
        }
        template<typename U>
        void make_room_right()
        {
            constexpr typename Tuple::Type layer_type = std::is_same<U, T>::value ? Tuple::LastLayer : Tuple::IntermediateLayer;
            Tuple one_layer_down(layer_type, 2);
            U * right_elems = static_cast<U *>(right.data.get());
            one_layer_down.push_back_right(std::move(right_elems[0]));
            one_layer_down.push_back_right(std::move(right_elems[1]));
            std::move(right_elems + 2, right_elems + SideCapacity, right_elems);
            right.size -= 2;
            if (middle)
                middle->push_right(std::move(one_layer_down));
            else
                middle.reset(new Seq(std::move(one_layer_down)));
        }

        void push_left(const T & elem)
        {
            measure = Measure::combine(Measure::measure(elem), std::move(measure));
            if (left.size == 0) // empty case
                left.push_back_left(elem);
            else if (right.size == 0)
            {
                // one elem case
                T * left_data = static_cast<T *>(left.data.get());
                right.push_back_right(std::move(*left_data));
                *left_data = elem;
            }
            else
            {
                if (left.size == SideCapacity)
                    make_room_left<T>();
                left.push_back_left(elem);
            }
        }
        void push_left(T && elem)
        {
            measure = Measure::combine(Measure::measure(elem), std::move(measure));
            if (left.size == 0) // empty case
                left.push_back_left(std::move(elem));
            else if (right.size == 0)
            {
                // one elem case
                T * left_data = static_cast<T *>(left.data.get());
                right.push_back_right(std::move(*left_data));
                *left_data = std::move(elem);
            }
            else
            {
                if (left.size == SideCapacity)
                    make_room_left<T>();
                left.push_back_left(std::move(elem));
            }
        }
        void push_left(Tuple && elem)
        {
            measure = Measure::combine(elem.measure, std::move(measure));
            CHECK_FOR_PROGRAMMER_ERROR(left.size > 0);
            if (right.size == 0)
            {
                // one elem case
                Tuple * left_data = static_cast<Tuple *>(left.data.get());
                right.push_back_right(std::move(*left_data));
                *left_data = std::move(elem);
            }
            else
            {
                if (left.size == SideCapacity)
                    make_room_left<Tuple>();
                left.push_back_left(std::move(elem));
            }
        }
        void push_right(const T & elem)
        {
            measure = Measure::combine(std::move(measure), Measure::measure(elem));
            if (left.size == 0) // empty case
                left.push_back_left(elem);
            else
            {
                if (right.size == SideCapacity)
                    make_room_right<T>();
                right.push_back_right(elem);
            }
        }
        void push_right(T && elem)
        {
            measure = Measure::combine(std::move(measure), Measure::measure(elem));
            if (left.size == 0) // empty case
                left.push_back_left(std::move(elem));
            else
            {
                if (right.size == SideCapacity)
                    make_room_right<T>();
                right.push_back_right(std::move(elem));
            }
        }
        void push_right(Tuple && elem)
        {
            measure = Measure::combine(std::move(measure), elem.measure);
            CHECK_FOR_PROGRAMMER_ERROR(left.size > 0);
            if (right.size == SideCapacity)
                make_room_right<Tuple>();
            right.push_back_right(std::move(elem));
        }

        template<typename U>
        U & leftmost()
        {
            return static_cast<U *>(left.data.get())[left.size - 1];
        }
        template<typename U>
        const U & leftmost() const
        {
            return static_cast<const U *>(left.data.get())[left.size - 1];
        }
        template<typename U>
        U & rightmost()
        {
            if (right.size)
                return static_cast<U *>(right.data.get())[right.size - 1];
            else
                return static_cast<U *>(left.data.get())[0];
        }
        template<typename U>
        const U & rightmost() const
        {
            if (right.size)
                return static_cast<const U *>(right.data.get())[right.size - 1];
            else
                return static_cast<const U *>(left.data.get())[0];
        }

        bool pop_left()
        {
            left.pop_back_left();
            if (middle)
                measure = Measure::combine(left.measure, Measure::combine(middle->measure, right.measure));
            else
                measure = Measure::combine(left.measure, right.measure);
            if (left.size != 0)
                return true;

            if (middle)
            {
                left = std::move(middle->template leftmost<Tuple>());
                if (left.type == Tuple::LastLayer)
                {
                    T * data = static_cast<T *>(left.data.get());
                    std::reverse(data, data + left.size);
                }
                else
                {
                    Tuple * data = static_cast<Tuple *>(left.data.get());
                    std::reverse(data, data + left.size);
                }
                if (!middle->pop_left())
                    middle.reset();
                return true;
            }
            else if (right.size)
            {
                if (right.type == Tuple::LastLayer)
                    left.push_back_left(std::move(*static_cast<T *>(right.data.get())));
                else
                    left.push_back_left(std::move(*static_cast<Tuple *>(right.data.get())));
                right.pop_front_right();
                return true;
            }
            else
                return false;
        }
        bool pop_right()
        {
            if (right.size == 0)
            {
                CHECK_FOR_PROGRAMMER_ERROR(left.size == 1);
                left.pop_back_left();
                measure = Measure::empty();
                return false;
            }
            right.pop_back_right();
            if (middle)
                measure = Measure::combine(left.measure, Measure::combine(middle->measure, right.measure));
            else
                measure = Measure::combine(left.measure, right.measure);
            if (right.size != 0)
                return true;
            else if (middle)
            {
                right = std::move(middle->template rightmost<Tuple>());
                if (!middle->pop_right())
                    middle.reset();
            }
            else if (left.size > 1)
            {
                if (left.type == Tuple::LastLayer)
                    right.push_back_right(std::move(*static_cast<T *>(left.data.get())));
                else
                    right.push_back_right(std::move(*static_cast<Tuple *>(left.data.get())));
                left.pop_front_left();
            }
            return true;
        }
    };

    Seq seq;

public:

    void push_left(const T & elem)
    {
        seq.push_left(elem);
    }
    void push_left(T && elem)
    {
        seq.push_left(std::move(elem));
    }
    void push_right(const T & elem)
    {
        seq.push_right(elem);
    }
    void push_right(T && elem)
    {
        seq.push_right(std::move(elem));
    }

    void pop_left()
    {
        seq.pop_left();
    }
    void pop_right()
    {
        seq.pop_right();
    }

    bool empty() const
    {
        return seq.left.size == 0;
    }
    T & left()
    {
        return seq.template leftmost<T>();
    }
    const T & left() const
    {
        return seq.template leftmost<T>();
    }
    T & right()
    {
        return seq.template rightmost<T>();
    }
    const T & right() const
    {
        return seq.template rightmost<T>();
    }

    template<typename F>
    void iter(F && f) const
    {
        seq.iter(f);
    }
    template<typename MeasureCheck, typename F>
    void iter_right_from(MeasureCheck && check, F && f)
    {
        seq.iter_right_from(check, f);
    }

    void print_tree(std::ostream & out) const
    {
        seq.print_tree(out, 0);
    }
};
