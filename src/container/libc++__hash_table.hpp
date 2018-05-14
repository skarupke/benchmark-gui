// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP__HASH_TABLE
#define _LIBCPP__HASH_TABLE

#include <initializer_list>
#include <memory>
#include <iterator>
#include <algorithm>
#include <cmath>
#include <x86intrin.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#pragma GCC system_header
#endif

namespace llvm_std
{


size_t __next_prime(size_t __n);

template <class _NodePtr>
struct __hash_node_base
{
    typedef __hash_node_base __first_node;

    _NodePtr    __next_;

    inline __hash_node_base() noexcept : __next_(nullptr) {}
};

template <class _Tp, class _VoidPtr>
struct __hash_node
    : public __hash_node_base
             <
                 typename std::pointer_traits<_VoidPtr>::template
#ifndef _LIBCPP_HAS_NO_TEMPLATE_ALIASES
                     rebind<__hash_node<_Tp, _VoidPtr> >
#else
                     rebind<__hash_node<_Tp, _VoidPtr> >::other
#endif
             >
{
    typedef _Tp value_type;

    size_t     __hash_;
    value_type __value_;
};

inline
bool
__is_hash_power2(size_t __bc)
{
    return __bc > 2 && !(__bc & (__bc - 1));
}

inline
size_t
__constrain_hash(size_t __h, size_t __bc)
{
    return !(__bc & (__bc - 1)) ? __h & (__bc - 1) : __h % __bc;
}

inline
size_t
__next_hash_pow2(size_t __n)
{
    return size_t(1) << (std::numeric_limits<size_t>::digits - _bit_scan_forward(__n-1));
}

template <class _Tp, class _Hash, class _Equal, class _Alloc> class __hash_table;
template <class _ConstNodePtr> class  __hash_const_iterator;
template <class _HashIterator> class  __hash_map_iterator;
template <class _HashIterator> class  __hash_map_const_iterator;

template <class _NodePtr>
class  __hash_iterator
{
    typedef _NodePtr __node_pointer;

    __node_pointer            __node_;

public:
    typedef std::forward_iterator_tag                         iterator_category;
    typedef typename std::pointer_traits<__node_pointer>::element_type::value_type value_type;
    typedef typename std::pointer_traits<__node_pointer>::difference_type difference_type;
    typedef value_type&                                  reference;
    typedef typename std::pointer_traits<__node_pointer>::template
#ifndef _LIBCPP_HAS_NO_TEMPLATE_ALIASES
                     rebind<value_type>
#else
                     rebind<value_type>::other
#endif
                                                         pointer;

     __hash_iterator() noexcept
#if _LIBCPP_STD_VER > 11
    : __node_(nullptr)
#endif
    {
#if _LIBCPP_DEBUG_LEVEL >= 2
        __get_db()->__insert_i(this);
#endif
    }

#if _LIBCPP_DEBUG_LEVEL >= 2


    __hash_iterator(const __hash_iterator& __i)
        : __node_(__i.__node_)
    {
        __get_db()->__iterator_copy(this, &__i);
    }


    ~__hash_iterator()
    {
        __get_db()->__erase_i(this);
    }


    __hash_iterator& operator=(const __hash_iterator& __i)
    {
        if (this != &__i)
        {
            __get_db()->__iterator_copy(this, &__i);
            __node_ = __i.__node_;
        }
        return *this;
    }

#endif  // _LIBCPP_DEBUG_LEVEL >= 2


        reference operator*() const
        {
#if _LIBCPP_DEBUG_LEVEL >= 2
            _LIBCPP_ASSERT(__get_const_db()->__dereferenceable(this),
                           "Attempted to dereference a non-dereferenceable unordered container iterator");
#endif
            return __node_->__value_;
        }

        pointer operator->() const
        {
#if _LIBCPP_DEBUG_LEVEL >= 2
            _LIBCPP_ASSERT(__get_const_db()->__dereferenceable(this),
                           "Attempted to dereference a non-dereferenceable unordered container iterator");
#endif
            return std::pointer_traits<pointer>::pointer_to(__node_->__value_);
        }


    __hash_iterator& operator++()
    {
#if _LIBCPP_DEBUG_LEVEL >= 2
        _LIBCPP_ASSERT(__get_const_db()->__dereferenceable(this),
                       "Attempted to increment non-incrementable unordered container iterator");
#endif
        __node_ = __node_->__next_;
        return *this;
    }


    __hash_iterator operator++(int)
    {
        __hash_iterator __t(*this);
        ++(*this);
        return __t;
    }

    friend
    bool operator==(const __hash_iterator& __x, const __hash_iterator& __y)
    {
        return __x.__node_ == __y.__node_;
    }
    friend
    bool operator!=(const __hash_iterator& __x, const __hash_iterator& __y)
        {return !(__x == __y);}

private:
#if _LIBCPP_DEBUG_LEVEL >= 2

    __hash_iterator(__node_pointer __node, const void* __c) noexcept
        : __node_(__node)
        {
            __get_db()->__insert_ic(this, __c);
        }
#else

    __hash_iterator(__node_pointer __node) noexcept
        : __node_(__node)
        {}
#endif

    template <class, class, class, class> friend class __hash_table;
    template <class> friend class  __hash_const_iterator;
    template <class> friend class  __hash_map_iterator;
    template <class, class, class, class, class> friend class  unordered_map;
    template <class, class, class, class, class> friend class  unordered_multimap;
};

template <class _ConstNodePtr>
class  __hash_const_iterator
{
    typedef _ConstNodePtr __node_pointer;

    __node_pointer         __node_;

    typedef typename std::remove_const<
        typename std::pointer_traits<__node_pointer>::element_type
                                 >::type __node;

public:
    typedef std::forward_iterator_tag                       iterator_category;
    typedef typename __node::value_type                value_type;
    typedef typename std::pointer_traits<__node_pointer>::difference_type difference_type;
    typedef const value_type&                          reference;
    typedef typename std::pointer_traits<__node_pointer>::template
#ifndef _LIBCPP_HAS_NO_TEMPLATE_ALIASES
            rebind<const value_type>
#else
            rebind<const value_type>::other
#endif
                                                       pointer;
    typedef typename std::pointer_traits<__node_pointer>::template
#ifndef _LIBCPP_HAS_NO_TEMPLATE_ALIASES
            rebind<__node>
#else
            rebind<__node>::other
#endif
                                                      __non_const_node_pointer;
    typedef __hash_iterator<__non_const_node_pointer> __non_const_iterator;

     __hash_const_iterator() noexcept
#if _LIBCPP_STD_VER > 11
    : __node_(nullptr)
#endif
    {
#if _LIBCPP_DEBUG_LEVEL >= 2
        __get_db()->__insert_i(this);
#endif
    }

    __hash_const_iterator(const __non_const_iterator& __x) noexcept
        : __node_(__x.__node_)
    {
#if _LIBCPP_DEBUG_LEVEL >= 2
        __get_db()->__iterator_copy(this, &__x);
#endif
    }

#if _LIBCPP_DEBUG_LEVEL >= 2


    __hash_const_iterator(const __hash_const_iterator& __i)
        : __node_(__i.__node_)
    {
        __get_db()->__iterator_copy(this, &__i);
    }


    ~__hash_const_iterator()
    {
        __get_db()->__erase_i(this);
    }


    __hash_const_iterator& operator=(const __hash_const_iterator& __i)
    {
        if (this != &__i)
        {
            __get_db()->__iterator_copy(this, &__i);
            __node_ = __i.__node_;
        }
        return *this;
    }

#endif  // _LIBCPP_DEBUG_LEVEL >= 2


        reference operator*() const
        {
#if _LIBCPP_DEBUG_LEVEL >= 2
            _LIBCPP_ASSERT(__get_const_db()->__dereferenceable(this),
                           "Attempted to dereference a non-dereferenceable unordered container const_iterator");
#endif
            return __node_->__value_;
        }

        pointer operator->() const
        {
#if _LIBCPP_DEBUG_LEVEL >= 2
            _LIBCPP_ASSERT(__get_const_db()->__dereferenceable(this),
                           "Attempted to dereference a non-dereferenceable unordered container const_iterator");
#endif
            return std::pointer_traits<pointer>::pointer_to(__node_->__value_);
        }


    __hash_const_iterator& operator++()
    {
#if _LIBCPP_DEBUG_LEVEL >= 2
        _LIBCPP_ASSERT(__get_const_db()->__dereferenceable(this),
                       "Attempted to increment non-incrementable unordered container const_iterator");
#endif
        __node_ = __node_->__next_;
        return *this;
    }


    __hash_const_iterator operator++(int)
    {
        __hash_const_iterator __t(*this);
        ++(*this);
        return __t;
    }

    friend
    bool operator==(const __hash_const_iterator& __x, const __hash_const_iterator& __y)
    {
        return __x.__node_ == __y.__node_;
    }
    friend
    bool operator!=(const __hash_const_iterator& __x, const __hash_const_iterator& __y)
        {return !(__x == __y);}

private:
#if _LIBCPP_DEBUG_LEVEL >= 2

    __hash_const_iterator(__node_pointer __node, const void* __c) noexcept
        : __node_(__node)
        {
            __get_db()->__insert_ic(this, __c);
        }
#else

    __hash_const_iterator(__node_pointer __node) noexcept
        : __node_(__node)
        {}
#endif

    template <class, class, class, class> friend class __hash_table;
    template <class> friend class  __hash_map_const_iterator;
    template <class, class, class, class, class> friend class  unordered_map;
    template <class, class, class, class, class> friend class  unordered_multimap;
};

template <class _ConstNodePtr> class  __hash_const_local_iterator;

template <class _NodePtr>
class  __hash_local_iterator
{
    typedef _NodePtr __node_pointer;

    __node_pointer         __node_;
    size_t                 __bucket_;
    size_t                 __bucket_count_;

    typedef std::pointer_traits<__node_pointer>          __pointer_traits;
public:
    typedef std::forward_iterator_tag                                iterator_category;
    typedef typename __pointer_traits::element_type::value_type value_type;
    typedef typename __pointer_traits::difference_type          difference_type;
    typedef value_type&                                         reference;
    typedef typename __pointer_traits::template
#ifndef _LIBCPP_HAS_NO_TEMPLATE_ALIASES
            rebind<value_type>
#else
            rebind<value_type>::other
#endif
                                                                pointer;

     __hash_local_iterator() noexcept
    {
#if _LIBCPP_DEBUG_LEVEL >= 2
        __get_db()->__insert_i(this);
#endif
    }

#if _LIBCPP_DEBUG_LEVEL >= 2


    __hash_local_iterator(const __hash_local_iterator& __i)
        : __node_(__i.__node_),
          __bucket_(__i.__bucket_),
          __bucket_count_(__i.__bucket_count_)
    {
        __get_db()->__iterator_copy(this, &__i);
    }


    ~__hash_local_iterator()
    {
        __get_db()->__erase_i(this);
    }


    __hash_local_iterator& operator=(const __hash_local_iterator& __i)
    {
        if (this != &__i)
        {
            __get_db()->__iterator_copy(this, &__i);
            __node_ = __i.__node_;
            __bucket_ = __i.__bucket_;
            __bucket_count_ = __i.__bucket_count_;
        }
        return *this;
    }

#endif  // _LIBCPP_DEBUG_LEVEL >= 2


        reference operator*() const
        {
#if _LIBCPP_DEBUG_LEVEL >= 2
            _LIBCPP_ASSERT(__get_const_db()->__dereferenceable(this),
                           "Attempted to dereference a non-dereferenceable unordered container local_iterator");
#endif
            return __node_->__value_;
        }

        pointer operator->() const
        {
#if _LIBCPP_DEBUG_LEVEL >= 2
            _LIBCPP_ASSERT(__get_const_db()->__dereferenceable(this),
                           "Attempted to dereference a non-dereferenceable unordered container local_iterator");
#endif
            return std::pointer_traits<pointer>::pointer_to(__node_->__value_);
        }


    __hash_local_iterator& operator++()
    {
#if _LIBCPP_DEBUG_LEVEL >= 2
        _LIBCPP_ASSERT(__get_const_db()->__dereferenceable(this),
                       "Attempted to increment non-incrementable unordered container local_iterator");
#endif
        __node_ = __node_->__next_;
        if (__node_ != nullptr && __constrain_hash(__node_->__hash_, __bucket_count_) != __bucket_)
            __node_ = nullptr;
        return *this;
    }


    __hash_local_iterator operator++(int)
    {
        __hash_local_iterator __t(*this);
        ++(*this);
        return __t;
    }

    friend
    bool operator==(const __hash_local_iterator& __x, const __hash_local_iterator& __y)
    {
        return __x.__node_ == __y.__node_;
    }
    friend
    bool operator!=(const __hash_local_iterator& __x, const __hash_local_iterator& __y)
        {return !(__x == __y);}

private:
#if _LIBCPP_DEBUG_LEVEL >= 2

    __hash_local_iterator(__node_pointer __node, size_t __bucket,
                          size_t __bucket_count, const void* __c) noexcept
        : __node_(__node),
          __bucket_(__bucket),
          __bucket_count_(__bucket_count)
        {
            __get_db()->__insert_ic(this, __c);
            if (__node_ != nullptr)
                __node_ = __node_->__next_;
        }
#else

    __hash_local_iterator(__node_pointer __node, size_t __bucket,
                          size_t __bucket_count) noexcept
        : __node_(__node),
          __bucket_(__bucket),
          __bucket_count_(__bucket_count)
        {
            if (__node_ != nullptr)
                __node_ = __node_->__next_;
        }
#endif
    template <class, class, class, class> friend class __hash_table;
    template <class> friend class  __hash_const_local_iterator;
    template <class> friend class  __hash_map_iterator;
};

template <class _ConstNodePtr>
class  __hash_const_local_iterator
{
    typedef _ConstNodePtr __node_pointer;

    __node_pointer         __node_;
    size_t                 __bucket_;
    size_t                 __bucket_count_;

    typedef std::pointer_traits<__node_pointer>          __pointer_traits;
    typedef typename __pointer_traits::element_type __node;
    typedef typename std::remove_const<__node>::type     __non_const_node;
    typedef typename std::pointer_traits<__node_pointer>::template
#ifndef _LIBCPP_HAS_NO_TEMPLATE_ALIASES
            rebind<__non_const_node>
#else
            rebind<__non_const_node>::other
#endif
                                                    __non_const_node_pointer;
    typedef __hash_local_iterator<__non_const_node_pointer>
                                                    __non_const_iterator;
public:
    typedef std::forward_iterator_tag                       iterator_category;
    typedef typename std::remove_const<
                        typename __pointer_traits::element_type::value_type
                     >::type                           value_type;
    typedef typename __pointer_traits::difference_type difference_type;
    typedef const value_type&                          reference;
    typedef typename __pointer_traits::template
#ifndef _LIBCPP_HAS_NO_TEMPLATE_ALIASES
            rebind<const value_type>
#else
            rebind<const value_type>::other
#endif
                                                       pointer;

     __hash_const_local_iterator() noexcept
    {
#if _LIBCPP_DEBUG_LEVEL >= 2
        __get_db()->__insert_i(this);
#endif
    }


    __hash_const_local_iterator(const __non_const_iterator& __x) noexcept
        : __node_(__x.__node_),
          __bucket_(__x.__bucket_),
          __bucket_count_(__x.__bucket_count_)
    {
#if _LIBCPP_DEBUG_LEVEL >= 2
        __get_db()->__iterator_copy(this, &__x);
#endif
    }

#if _LIBCPP_DEBUG_LEVEL >= 2


    __hash_const_local_iterator(const __hash_const_local_iterator& __i)
        : __node_(__i.__node_),
          __bucket_(__i.__bucket_),
          __bucket_count_(__i.__bucket_count_)
    {
        __get_db()->__iterator_copy(this, &__i);
    }


    ~__hash_const_local_iterator()
    {
        __get_db()->__erase_i(this);
    }


    __hash_const_local_iterator& operator=(const __hash_const_local_iterator& __i)
    {
        if (this != &__i)
        {
            __get_db()->__iterator_copy(this, &__i);
            __node_ = __i.__node_;
            __bucket_ = __i.__bucket_;
            __bucket_count_ = __i.__bucket_count_;
        }
        return *this;
    }

#endif  // _LIBCPP_DEBUG_LEVEL >= 2


        reference operator*() const
        {
#if _LIBCPP_DEBUG_LEVEL >= 2
            _LIBCPP_ASSERT(__get_const_db()->__dereferenceable(this),
                           "Attempted to dereference a non-dereferenceable unordered container const_local_iterator");
#endif
            return __node_->__value_;
        }

        pointer operator->() const
        {
#if _LIBCPP_DEBUG_LEVEL >= 2
            _LIBCPP_ASSERT(__get_const_db()->__dereferenceable(this),
                           "Attempted to dereference a non-dereferenceable unordered container const_local_iterator");
#endif
            return std::pointer_traits<pointer>::pointer_to(__node_->__value_);
        }


    __hash_const_local_iterator& operator++()
    {
#if _LIBCPP_DEBUG_LEVEL >= 2
        _LIBCPP_ASSERT(__get_const_db()->__dereferenceable(this),
                       "Attempted to increment non-incrementable unordered container const_local_iterator");
#endif
        __node_ = __node_->__next_;
        if (__node_ != nullptr && __constrain_hash(__node_->__hash_, __bucket_count_) != __bucket_)
            __node_ = nullptr;
        return *this;
    }


    __hash_const_local_iterator operator++(int)
    {
        __hash_const_local_iterator __t(*this);
        ++(*this);
        return __t;
    }

    friend
    bool operator==(const __hash_const_local_iterator& __x, const __hash_const_local_iterator& __y)
    {
        return __x.__node_ == __y.__node_;
    }
    friend
    bool operator!=(const __hash_const_local_iterator& __x, const __hash_const_local_iterator& __y)
        {return !(__x == __y);}

private:
#if _LIBCPP_DEBUG_LEVEL >= 2

    __hash_const_local_iterator(__node_pointer __node, size_t __bucket,
                                size_t __bucket_count, const void* __c) noexcept
        : __node_(__node),
          __bucket_(__bucket),
          __bucket_count_(__bucket_count)
        {
            __get_db()->__insert_ic(this, __c);
            if (__node_ != nullptr)
                __node_ = __node_->__next_;
        }
#else

    __hash_const_local_iterator(__node_pointer __node, size_t __bucket,
                                size_t __bucket_count) noexcept
        : __node_(__node),
          __bucket_(__bucket),
          __bucket_count_(__bucket_count)
        {
            if (__node_ != nullptr)
                __node_ = __node_->__next_;
        }
#endif
    template <class, class, class, class> friend class __hash_table;
    template <class> friend class  __hash_map_const_iterator;
};

template <class _Alloc>
class __bucket_list_deallocator
{
    typedef _Alloc                                          allocator_type;
    typedef std::allocator_traits<allocator_type>                __alloc_traits;
    typedef typename __alloc_traits::size_type              size_type;

    std::tuple<size_type, allocator_type> __data_;
public:
    typedef typename __alloc_traits::pointer pointer;


    __bucket_list_deallocator()
        noexcept(std::is_nothrow_default_constructible<allocator_type>::value)
        : __data_(0, {}) {}


    __bucket_list_deallocator(const allocator_type& __a, size_type __size)
        noexcept(std::is_nothrow_copy_constructible<allocator_type>::value)
        : __data_(__size, __a) {}

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES


    __bucket_list_deallocator(__bucket_list_deallocator&& __x)
        noexcept(std::is_nothrow_move_constructible<allocator_type>::value)
        : __data_(std::move(__x.__data_))
    {
        __x.size() = 0;
    }

#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES


    size_type& size() noexcept {return std::get<0>(__data_);}

    size_type  size() const noexcept {return std::get<0>(__data_);}


    allocator_type& __alloc() noexcept {return std::get<1>(__data_);}

    const allocator_type& __alloc() const noexcept {return std::get<1>(__data_);}


    void operator()(pointer __p) noexcept
    {
        __alloc_traits::deallocate(__alloc(), __p, size());
    }
};

template <class _Alloc> class __hash_map_node_destructor;

template <class _Alloc>
class __hash_node_destructor
{
    typedef _Alloc                                          allocator_type;
    typedef std::allocator_traits<allocator_type>                __alloc_traits;
    typedef typename __alloc_traits::value_type::value_type value_type;
public:
    typedef typename __alloc_traits::pointer                pointer;
private:

    allocator_type& __na_;

    __hash_node_destructor& operator=(const __hash_node_destructor&);

public:
    bool __value_constructed;


    explicit __hash_node_destructor(allocator_type& __na,
                                    bool __constructed = false) noexcept
        : __na_(__na),
          __value_constructed(__constructed)
        {}


    void operator()(pointer __p) noexcept
    {
        if (__value_constructed)
            __alloc_traits::destroy(__na_, std::addressof(__p->__value_));
        if (__p)
            __alloc_traits::deallocate(__na_, __p, 1);
    }

    template <class> friend class __hash_map_node_destructor;
};

template <class _Tp, class _Hash, class _Equal, class _Alloc>
class __hash_table
{
public:
    typedef _Tp    value_type;
    typedef _Hash  hasher;
    typedef _Equal key_equal;
    typedef _Alloc allocator_type;

private:
    typedef std::allocator_traits<allocator_type> __alloc_traits;
public:
    typedef value_type&                              reference;
    typedef const value_type&                        const_reference;
    typedef typename __alloc_traits::pointer         pointer;
    typedef typename __alloc_traits::const_pointer   const_pointer;
    typedef typename __alloc_traits::size_type       size_type;
    typedef typename __alloc_traits::difference_type difference_type;
public:
    // Create __node
    typedef __hash_node<value_type, typename __alloc_traits::void_pointer> __node;
    typedef typename __alloc_traits::template rebind_alloc<__node> __node_allocator;
    typedef std::allocator_traits<__node_allocator>       __node_traits;
    typedef typename __node_traits::pointer          __node_pointer;
    typedef typename __node_traits::pointer          __node_const_pointer;
    typedef __hash_node_base<__node_pointer>         __first_node;
    typedef typename std::pointer_traits<__node_pointer>::template
#ifndef _LIBCPP_HAS_NO_TEMPLATE_ALIASES
            rebind<__first_node>
#else
            rebind<__first_node>::other
#endif
                                                     __node_base_pointer;

private:

    typedef typename __node_traits::template rebind_alloc<__node_pointer> __pointer_allocator;
    typedef __bucket_list_deallocator<__pointer_allocator> __bucket_list_deleter;
    typedef std::unique_ptr<__node_pointer[], __bucket_list_deleter> __bucket_list;
    typedef std::allocator_traits<__pointer_allocator>          __pointer_alloc_traits;
    typedef typename __bucket_list_deleter::pointer __node_pointer_pointer;

    // --- Member data begin ---
    __bucket_list                                     __bucket_list_;
    std::tuple<__first_node, __node_allocator> __p1_;
    std::tuple<size_type, hasher>              __p2_;
    std::tuple<float, key_equal>               __p3_;
    // --- Member data end ---


    size_type& size() noexcept {return std::get<0>(__p2_);}
public:

    size_type  size() const noexcept {return std::get<0>(__p2_);}


    hasher& hash_function() noexcept {return std::get<1>(__p2_);}

    const hasher& hash_function() const noexcept {return std::get<1>(__p2_);}


    float& max_load_factor() noexcept {return std::get<0>(__p3_);}

    float  max_load_factor() const noexcept {return std::get<0>(__p3_);}


    key_equal& key_eq() noexcept {return std::get<1>(__p3_);}

    const key_equal& key_eq() const noexcept {return std::get<1>(__p3_);}


    __node_allocator& __node_alloc() noexcept {return std::get<1>(__p1_);}

    const __node_allocator& __node_alloc() const noexcept
        {return std::get<1>(__p1_);}

public:
    typedef __hash_iterator<__node_pointer>                   iterator;
    typedef __hash_const_iterator<__node_pointer>             const_iterator;
    typedef __hash_local_iterator<__node_pointer>             local_iterator;
    typedef __hash_const_local_iterator<__node_pointer>       const_local_iterator;

    __hash_table()
        noexcept(
            std::is_nothrow_default_constructible<__bucket_list>::value &&
            std::is_nothrow_default_constructible<__first_node>::value &&
            std::is_nothrow_default_constructible<__node_allocator>::value &&
            std::is_nothrow_default_constructible<hasher>::value &&
            std::is_nothrow_default_constructible<key_equal>::value);
    __hash_table(const hasher& __hf, const key_equal& __eql);
    __hash_table(const hasher& __hf, const key_equal& __eql,
                 const allocator_type& __a);
    explicit __hash_table(const allocator_type& __a);
    __hash_table(const __hash_table& __u);
    __hash_table(const __hash_table& __u, const allocator_type& __a);
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
    __hash_table(__hash_table&& __u)
        noexcept(
            std::is_nothrow_move_constructible<__bucket_list>::value &&
            std::is_nothrow_move_constructible<__first_node>::value &&
            std::is_nothrow_move_constructible<__node_allocator>::value &&
            std::is_nothrow_move_constructible<hasher>::value &&
            std::is_nothrow_move_constructible<key_equal>::value);
    __hash_table(__hash_table&& __u, const allocator_type& __a);
#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES
    ~__hash_table();

    __hash_table& operator=(const __hash_table& __u);
#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
    __hash_table& operator=(__hash_table&& __u)
        noexcept(
            __node_traits::propagate_on_container_move_assignment::value &&
            std::is_nothrow_move_assignable<__node_allocator>::value &&
            std::is_nothrow_move_assignable<hasher>::value &&
            std::is_nothrow_move_assignable<key_equal>::value);
#endif
    template <class _InputIterator>
        void __assign_unique(_InputIterator __first, _InputIterator __last);
    template <class _InputIterator>
        void __assign_multi(_InputIterator __first, _InputIterator __last);


    size_type max_size() const noexcept
    {
        return std::allocator_traits<__pointer_allocator>::max_size(
            __bucket_list_.get_deleter().__alloc());
    }

    std::pair<iterator, bool> __node_insert_unique(__node_pointer __nd);
    iterator             __node_insert_multi(__node_pointer __nd);
    iterator             __node_insert_multi(const_iterator __p,
                                             __node_pointer __nd);

#if !defined(_LIBCPP_HAS_NO_RVALUE_REFERENCES) && !defined(_LIBCPP_HAS_NO_VARIADICS)
    template <class... _Args>
        std::pair<iterator, bool> __emplace_unique(_Args&&... __args);
    template <class... _Args>
        iterator __emplace_multi(_Args&&... __args);
    template <class... _Args>
        iterator __emplace_hint_multi(const_iterator __p, _Args&&... __args);
#endif  // !defined(_LIBCPP_HAS_NO_RVALUE_REFERENCES) && !defined(_LIBCPP_HAS_NO_VARIADICS)

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
    template <class _ValueTp>

    std::pair<iterator, bool> __insert_unique_value(_ValueTp&& __x);
#else

    std::pair<iterator, bool> __insert_unique_value(const value_type& __x);
#endif

    std::pair<iterator, bool> __insert_unique(const value_type& __x);

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
    std::pair<iterator, bool> __insert_unique(value_type&& __x);
    template <class _Pp>
    std::pair<iterator, bool> __insert_unique(_Pp&& __x);
#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
    template <class _Pp>
        iterator __insert_multi(_Pp&& __x);
    template <class _Pp>
        iterator __insert_multi(const_iterator __p, _Pp&& __x);
#else  // _LIBCPP_HAS_NO_RVALUE_REFERENCES
    iterator __insert_multi(const value_type& __x);
    iterator __insert_multi(const_iterator __p, const value_type& __x);
#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

    void clear() noexcept;
    void rehash(size_type __n);
     void reserve(size_type __n)
        {rehash(static_cast<size_type>(ceil(__n / max_load_factor())));}


    size_type bucket_count() const noexcept
    {
        return __bucket_list_.get_deleter().size();
    }

    iterator       begin() noexcept;
    iterator       end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;

    template <class _Key>

        size_type bucket(const _Key& __k) const
        {
            _LIBCPP_ASSERT(bucket_count() > 0,
                "unordered container::bucket(key) called when bucket_count() == 0");
            return __constrain_hash(hash_function()(__k), bucket_count());
        }

    template <class _Key>
        inline iterator       find(const _Key& __x);
    template <class _Key>
        inline const_iterator find(const _Key& __x) const;

    typedef __hash_node_destructor<__node_allocator> _Dp;
    typedef std::unique_ptr<__node, _Dp> __node_holder;

    iterator erase(const_iterator __p);
    iterator erase(const_iterator __first, const_iterator __last);
    template <class _Key>
        size_type __erase_unique(const _Key& __k);
    template <class _Key>
        size_type __erase_multi(const _Key& __k);
    __node_holder remove(const_iterator __p) noexcept;

    template <class _Key>
        size_type __count_unique(const _Key& __k) const;
    template <class _Key>
        size_type __count_multi(const _Key& __k) const;

    template <class _Key>
        std::pair<iterator, iterator>
        __equal_range_unique(const _Key& __k);
    template <class _Key>
        std::pair<const_iterator, const_iterator>
        __equal_range_unique(const _Key& __k) const;

    template <class _Key>
        std::pair<iterator, iterator>
        __equal_range_multi(const _Key& __k);
    template <class _Key>
        std::pair<const_iterator, const_iterator>
        __equal_range_multi(const _Key& __k) const;

    void swap(__hash_table& __u)
        noexcept;


    size_type max_bucket_count() const noexcept
        {return __pointer_alloc_traits::max_size(__bucket_list_.get_deleter().__alloc());}
    size_type bucket_size(size_type __n) const;
     float load_factor() const noexcept
    {
        size_type __bc = bucket_count();
        return __bc != 0 ? (float)size() / __bc : 0.f;
    }
     void max_load_factor(float __mlf) noexcept
    {
        max_load_factor() = std::max(__mlf, load_factor());
    }


    local_iterator
    begin(size_type __n)
    {
        _LIBCPP_ASSERT(__n < bucket_count(),
            "unordered container::begin(n) called with n >= bucket_count()");
#if _LIBCPP_DEBUG_LEVEL >= 2
        return local_iterator(__bucket_list_[__n], __n, bucket_count(), this);
#else
        return local_iterator(__bucket_list_[__n], __n, bucket_count());
#endif
    }


    local_iterator
    end(size_type __n)
    {
        _LIBCPP_ASSERT(__n < bucket_count(),
            "unordered container::end(n) called with n >= bucket_count()");
#if _LIBCPP_DEBUG_LEVEL >= 2
        return local_iterator(nullptr, __n, bucket_count(), this);
#else
        return local_iterator(nullptr, __n, bucket_count());
#endif
    }


    const_local_iterator
    cbegin(size_type __n) const
    {
        _LIBCPP_ASSERT(__n < bucket_count(),
            "unordered container::cbegin(n) called with n >= bucket_count()");
#if _LIBCPP_DEBUG_LEVEL >= 2
        return const_local_iterator(__bucket_list_[__n], __n, bucket_count(), this);
#else
        return const_local_iterator(__bucket_list_[__n], __n, bucket_count());
#endif
    }


    const_local_iterator
    cend(size_type __n) const
    {
        _LIBCPP_ASSERT(__n < bucket_count(),
            "unordered container::cend(n) called with n >= bucket_count()");
#if _LIBCPP_DEBUG_LEVEL >= 2
        return const_local_iterator(nullptr, __n, bucket_count(), this);
#else
        return const_local_iterator(nullptr, __n, bucket_count());
#endif
    }

#if _LIBCPP_DEBUG_LEVEL >= 2

    bool __dereferenceable(const const_iterator* __i) const;
    bool __decrementable(const const_iterator* __i) const;
    bool __addable(const const_iterator* __i, ptrdiff_t __n) const;
    bool __subscriptable(const const_iterator* __i, ptrdiff_t __n) const;

#endif  // _LIBCPP_DEBUG_LEVEL >= 2

private:
    void __rehash(size_type __n);

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
#ifndef _LIBCPP_HAS_NO_VARIADICS
    template <class ..._Args>
        __node_holder __construct_node(_Args&& ...__args);
#endif  // _LIBCPP_HAS_NO_VARIADICS
    __node_holder __construct_node(value_type&& __v, size_t __hash);
#else  // _LIBCPP_HAS_NO_RVALUE_REFERENCES
    __node_holder __construct_node(const value_type& __v);
#endif
    __node_holder __construct_node(const value_type& __v, size_t __hash);


    void __copy_assign_alloc(const __hash_table& __u)
        {__copy_assign_alloc(__u, std::integral_constant<bool,
             __node_traits::propagate_on_container_copy_assignment::value>());}
    void __copy_assign_alloc(const __hash_table& __u, std::true_type);

        void __copy_assign_alloc(const __hash_table&, std::false_type) {}

    void __move_assign(__hash_table& __u, std::false_type);
    void __move_assign(__hash_table& __u, std::true_type)
        noexcept(
            std::is_nothrow_move_assignable<__node_allocator>::value &&
            std::is_nothrow_move_assignable<hasher>::value &&
            std::is_nothrow_move_assignable<key_equal>::value);

    void __move_assign_alloc(__hash_table& __u)
        noexcept(
            !__node_traits::propagate_on_container_move_assignment::value ||
            (std::is_nothrow_move_assignable<__pointer_allocator>::value &&
             std::is_nothrow_move_assignable<__node_allocator>::value))
        {__move_assign_alloc(__u, std::integral_constant<bool,
             __node_traits::propagate_on_container_move_assignment::value>());}

    void __move_assign_alloc(__hash_table& __u, std::true_type)
        noexcept(
            std::is_nothrow_move_assignable<__pointer_allocator>::value &&
            std::is_nothrow_move_assignable<__node_allocator>::value)
    {
        __bucket_list_.get_deleter().__alloc() =
                std::move(__u.__bucket_list_.get_deleter().__alloc());
        __node_alloc() = std::move(__u.__node_alloc());
    }

        void __move_assign_alloc(__hash_table&, std::false_type) noexcept {}

    void __deallocate(__node_pointer __np) noexcept;
    __node_pointer __detach() noexcept;

    template <class, class, class, class, class> friend class  unordered_map;
    template <class, class, class, class, class> friend class  unordered_multimap;
};

template <class _Tp, class _Hash, class _Equal, class _Alloc>
inline
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__hash_table()
    noexcept(
        std::is_nothrow_default_constructible<__bucket_list>::value &&
        std::is_nothrow_default_constructible<__first_node>::value &&
        std::is_nothrow_default_constructible<__node_allocator>::value &&
        std::is_nothrow_default_constructible<hasher>::value &&
        std::is_nothrow_default_constructible<key_equal>::value)
    : __p2_(0, {}),
      __p3_(1.0f, {})
{
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
inline
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__hash_table(const hasher& __hf,
                                                       const key_equal& __eql)
    : __bucket_list_(nullptr, __bucket_list_deleter()),
      __p1_(),
      __p2_(0, __hf),
      __p3_(1.0f, __eql)
{
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__hash_table(const hasher& __hf,
                                                       const key_equal& __eql,
                                                       const allocator_type& __a)
    : __bucket_list_(nullptr, __bucket_list_deleter(__pointer_allocator(__a), 0)),
      __p1_(__node_allocator(__a)),
      __p2_(0, __hf),
      __p3_(1.0f, __eql)
{
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__hash_table(const allocator_type& __a)
    : __bucket_list_(nullptr, __bucket_list_deleter(__pointer_allocator(__a), 0)),
      __p1_(__node_allocator(__a)),
      __p2_(0, {}),
      __p3_(1.0f, {})
{
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__hash_table(const __hash_table& __u)
    : __bucket_list_(nullptr,
          __bucket_list_deleter(std::allocator_traits<__pointer_allocator>::
              select_on_container_copy_construction(
                  __u.__bucket_list_.get_deleter().__alloc()), 0)),
      __p1_(__first_node(), std::allocator_traits<__node_allocator>::
          select_on_container_copy_construction(__u.__node_alloc())),
      __p2_(0, __u.hash_function()),
      __p3_(__u.__p3_)
{
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__hash_table(const __hash_table& __u,
                                                       const allocator_type& __a)
    : __bucket_list_(nullptr, __bucket_list_deleter(__pointer_allocator(__a), 0)),
      __p1_(__first_node(), __node_allocator(__a)),
      __p2_(0, __u.hash_function()),
      __p3_(__u.__p3_)
{
}

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _Tp, class _Hash, class _Equal, class _Alloc>
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__hash_table(__hash_table&& __u)
        noexcept(
            std::is_nothrow_move_constructible<__bucket_list>::value &&
            std::is_nothrow_move_constructible<__first_node>::value &&
            std::is_nothrow_move_constructible<__node_allocator>::value &&
            std::is_nothrow_move_constructible<hasher>::value &&
            std::is_nothrow_move_constructible<key_equal>::value)
    : __bucket_list_(std::move(__u.__bucket_list_)),
      __p1_(std::move(__u.__p1_)),
      __p2_(std::move(__u.__p2_)),
      __p3_(std::move(__u.__p3_))
{
    if (size() > 0)
    {
        __bucket_list_[__constrain_hash(std::get<0>(__p1_).__next_->__hash_, bucket_count())] =
            static_cast<__node_pointer>(std::pointer_traits<__node_base_pointer>::pointer_to(std::get<0>(__p1_)));
        std::get<0>(__u.__p1_).__next_ = nullptr;
        __u.size() = 0;
    }
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__hash_table(__hash_table&& __u,
                                                       const allocator_type& __a)
    : __bucket_list_(nullptr, __bucket_list_deleter(__pointer_allocator(__a), 0)),
      __p1_(__node_allocator(__a)),
      __p2_(0, std::move(__u.hash_function())),
      __p3_(std::move(__u.__p3_))
{
    if (__a == allocator_type(__u.__node_alloc()))
    {
        __bucket_list_.reset(__u.__bucket_list_.release());
        __bucket_list_.get_deleter().size() = __u.__bucket_list_.get_deleter().size();
        __u.__bucket_list_.get_deleter().size() = 0;
        if (__u.size() > 0)
        {
            std::get<0>(__p1_).__next_ = std::get<0>(__u.__p1_).__next_;
            std::get<0>(__u.__p1_).__next_ = nullptr;
            __bucket_list_[__constrain_hash(std::get<0>(__p1_).__next_->__hash_, bucket_count())] =
                static_cast<__node_pointer>(std::pointer_traits<__node_base_pointer>::pointer_to(std::get<0>(__p1_)));
            size() = __u.size();
            __u.size() = 0;
        }
    }
}

#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _Tp, class _Hash, class _Equal, class _Alloc>
__hash_table<_Tp, _Hash, _Equal, _Alloc>::~__hash_table()
{
    __deallocate(std::get<0>(__p1_).__next_);
#if _LIBCPP_DEBUG_LEVEL >= 2
    __get_db()->__erase_c(this);
#endif
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
void
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__copy_assign_alloc(
        const __hash_table& __u, std::true_type)
{
    if (__node_alloc() != __u.__node_alloc())
    {
        clear();
        __bucket_list_.reset();
        __bucket_list_.get_deleter().size() = 0;
    }
    __bucket_list_.get_deleter().__alloc() = __u.__bucket_list_.get_deleter().__alloc();
    __node_alloc() = __u.__node_alloc();
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
__hash_table<_Tp, _Hash, _Equal, _Alloc>&
__hash_table<_Tp, _Hash, _Equal, _Alloc>::operator=(const __hash_table& __u)
{
    if (this != &__u)
    {
        __copy_assign_alloc(__u);
        hash_function() = __u.hash_function();
        key_eq() = __u.key_eq();
        max_load_factor() = __u.max_load_factor();
        __assign_multi(__u.begin(), __u.end());
    }
    return *this;
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
void
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__deallocate(__node_pointer __np)
    noexcept
{
    __node_allocator& __na = __node_alloc();
    while (__np != nullptr)
    {
        __node_pointer __next = __np->__next_;
#if _LIBCPP_DEBUG_LEVEL >= 2
        __c_node* __c = __get_db()->__find_c_and_lock(this);
        for (__i_node** __p = __c->end_; __p != __c->beg_; )
        {
            --__p;
            iterator* __i = static_cast<iterator*>((*__p)->__i_);
            if (__i->__node_ == __np)
            {
                (*__p)->__c_ = nullptr;
                if (--__c->end_ != __p)
                    memmove(__p, __p+1, (__c->end_ - __p)*sizeof(__i_node*));
            }
        }
        __get_db()->unlock();
#endif
        __node_traits::destroy(__na, std::addressof(__np->__value_));
        __node_traits::deallocate(__na, __np, 1);
        __np = __next;
    }
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::__node_pointer
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__detach() noexcept
{
    size_type __bc = bucket_count();
    for (size_type __i = 0; __i < __bc; ++__i)
        __bucket_list_[__i] = nullptr;
    size() = 0;
    __node_pointer __cache = std::get<0>(__p1_).__next_;
    std::get<0>(__p1_).__next_ = nullptr;
    return __cache;
}

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _Tp, class _Hash, class _Equal, class _Alloc>
void
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__move_assign(
        __hash_table& __u, std::true_type)
    noexcept(
        std::is_nothrow_move_assignable<__node_allocator>::value &&
        std::is_nothrow_move_assignable<hasher>::value &&
        std::is_nothrow_move_assignable<key_equal>::value)
{
    clear();
    __bucket_list_.reset(__u.__bucket_list_.release());
    __bucket_list_.get_deleter().size() = __u.__bucket_list_.get_deleter().size();
    __u.__bucket_list_.get_deleter().size() = 0;
    __move_assign_alloc(__u);
    size() = __u.size();
    hash_function() = std::move(__u.hash_function());
    max_load_factor() = __u.max_load_factor();
    key_eq() = std::move(__u.key_eq());
    std::get<0>(__p1_).__next_ = std::get<0>(__u.__p1_).__next_;
    if (size() > 0)
    {
        __bucket_list_[__constrain_hash(std::get<0>(__p1_).__next_->__hash_, bucket_count())] =
            static_cast<__node_pointer>(std::pointer_traits<__node_base_pointer>::pointer_to(std::get<0>(__p1_)));
        std::get<0>(__u.__p1_).__next_ = nullptr;
        __u.size() = 0;
    }
#if _LIBCPP_DEBUG_LEVEL >= 2
    __get_db()->swap(this, &__u);
#endif
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
void
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__move_assign(
        __hash_table& __u, std::false_type)
{
    if (__node_alloc() == __u.__node_alloc())
        __move_assign(__u, std::true_type());
    else
    {
        hash_function() = std::move(__u.hash_function());
        key_eq() = std::move(__u.key_eq());
        max_load_factor() = __u.max_load_factor();
        if (bucket_count() != 0)
        {
            __node_pointer __cache = __detach();
#ifndef _LIBCPP_NO_EXCEPTIONS
            try
            {
#endif  // _LIBCPP_NO_EXCEPTIONS
                const_iterator __i = __u.begin();
                while (__cache != nullptr && __u.size() != 0)
                {
                    __cache->__value_ = std::move(__u.remove(__i++)->__value_);
                    __node_pointer __next = __cache->__next_;
                    __node_insert_multi(__cache);
                    __cache = __next;
                }
#ifndef _LIBCPP_NO_EXCEPTIONS
            }
            catch (...)
            {
                __deallocate(__cache);
                throw;
            }
#endif  // _LIBCPP_NO_EXCEPTIONS
            __deallocate(__cache);
        }
        const_iterator __i = __u.begin();
        while (__u.size() != 0)
        {
            __node_holder __h =
                    __construct_node(std::move(__u.remove(__i++)->__value_));
            __node_insert_multi(__h.get());
            __h.release();
        }
    }
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
inline
__hash_table<_Tp, _Hash, _Equal, _Alloc>&
__hash_table<_Tp, _Hash, _Equal, _Alloc>::operator=(__hash_table&& __u)
    noexcept(
        __node_traits::propagate_on_container_move_assignment::value &&
        std::is_nothrow_move_assignable<__node_allocator>::value &&
        std::is_nothrow_move_assignable<hasher>::value &&
        std::is_nothrow_move_assignable<key_equal>::value)
{
    __move_assign(__u, std::integral_constant<bool,
                  __node_traits::propagate_on_container_move_assignment::value>());
    return *this;
}

#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class _InputIterator>
void
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__assign_unique(_InputIterator __first,
                                                          _InputIterator __last)
{
    if (bucket_count() != 0)
    {
        __node_pointer __cache = __detach();
#ifndef _LIBCPP_NO_EXCEPTIONS
        try
        {
#endif  // _LIBCPP_NO_EXCEPTIONS
            for (; __cache != nullptr && __first != __last; ++__first)
            {
                __cache->__value_ = *__first;
                __node_pointer __next = __cache->__next_;
                __node_insert_unique(__cache);
                __cache = __next;
            }
#ifndef _LIBCPP_NO_EXCEPTIONS
        }
        catch (...)
        {
            __deallocate(__cache);
            throw;
        }
#endif  // _LIBCPP_NO_EXCEPTIONS
        __deallocate(__cache);
    }
    for (; __first != __last; ++__first)
        __insert_unique(*__first);
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class _InputIterator>
void
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__assign_multi(_InputIterator __first,
                                                         _InputIterator __last)
{
    if (bucket_count() != 0)
    {
        __node_pointer __cache = __detach();
#ifndef _LIBCPP_NO_EXCEPTIONS
        try
        {
#endif  // _LIBCPP_NO_EXCEPTIONS
            for (; __cache != nullptr && __first != __last; ++__first)
            {
                __cache->__value_ = *__first;
                __node_pointer __next = __cache->__next_;
                __node_insert_multi(__cache);
                __cache = __next;
            }
#ifndef _LIBCPP_NO_EXCEPTIONS
        }
        catch (...)
        {
            __deallocate(__cache);
            throw;
        }
#endif  // _LIBCPP_NO_EXCEPTIONS
        __deallocate(__cache);
    }
    for (; __first != __last; ++__first)
        __insert_multi(*__first);
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
inline
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator
__hash_table<_Tp, _Hash, _Equal, _Alloc>::begin() noexcept
{
#if _LIBCPP_DEBUG_LEVEL >= 2
    return iterator(std::get<0>(__p1_).__next_, this);
#else
    return iterator(std::get<0>(__p1_).__next_);
#endif
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
inline
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator
__hash_table<_Tp, _Hash, _Equal, _Alloc>::end() noexcept
{
#if _LIBCPP_DEBUG_LEVEL >= 2
    return iterator(nullptr, this);
#else
    return iterator(nullptr);
#endif
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
inline
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::const_iterator
__hash_table<_Tp, _Hash, _Equal, _Alloc>::begin() const noexcept
{
#if _LIBCPP_DEBUG_LEVEL >= 2
    return const_iterator(std::get<0>(__p1_).__next_, this);
#else
    return const_iterator(std::get<0>(__p1_).__next_);
#endif
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
inline
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::const_iterator
__hash_table<_Tp, _Hash, _Equal, _Alloc>::end() const noexcept
{
#if _LIBCPP_DEBUG_LEVEL >= 2
    return const_iterator(nullptr, this);
#else
    return const_iterator(nullptr);
#endif
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
void
__hash_table<_Tp, _Hash, _Equal, _Alloc>::clear() noexcept
{
    if (size() > 0)
    {
        __deallocate(std::get<0>(__p1_).__next_);
        std::get<0>(__p1_).__next_ = nullptr;
        size_type __bc = bucket_count();
        for (size_type __i = 0; __i < __bc; ++__i)
            __bucket_list_[__i] = nullptr;
        size() = 0;
    }
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
std::pair<typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator, bool>
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__node_insert_unique(__node_pointer __nd)
{
    __nd->__hash_ = hash_function()(__nd->__value_);
    size_type __bc = bucket_count();
    bool __inserted = false;
    __node_pointer __ndptr;
    size_t __chash;
    if (__bc != 0)
    {
        __chash = __constrain_hash(__nd->__hash_, __bc);
        __ndptr = __bucket_list_[__chash];
        if (__ndptr != nullptr)
        {
            for (__ndptr = __ndptr->__next_; __ndptr != nullptr &&
                                             __constrain_hash(__ndptr->__hash_, __bc) == __chash;
                                                     __ndptr = __ndptr->__next_)
            {
                if (key_eq()(__ndptr->__value_, __nd->__value_))
                    goto __done;
            }
        }
    }
    {
        if (size()+1 > __bc * max_load_factor() || __bc == 0)
        {
            rehash(std::max<size_type>(2 * __bc + !__is_hash_power2(__bc),
                           size_type(ceil(float(size() + 1) / max_load_factor()))));
            __bc = bucket_count();
            __chash = __constrain_hash(__nd->__hash_, __bc);
        }
        // insert_after __bucket_list_[__chash], or __first_node if bucket is null
        __node_pointer __pn = __bucket_list_[__chash];
        if (__pn == nullptr)
        {
            __pn = static_cast<__node_pointer>(std::pointer_traits<__node_base_pointer>::pointer_to(std::get<0>(__p1_)));
            __nd->__next_ = __pn->__next_;
            __pn->__next_ = __nd;
            // fix up __bucket_list_
            __bucket_list_[__chash] = __pn;
            if (__nd->__next_ != nullptr)
                __bucket_list_[__constrain_hash(__nd->__next_->__hash_, __bc)] = __nd;
        }
        else
        {
            __nd->__next_ = __pn->__next_;
            __pn->__next_ = __nd;
        }
        __ndptr = __nd;
        // increment size
        ++size();
        __inserted = true;
    }
__done:
#if _LIBCPP_DEBUG_LEVEL >= 2
    return std::pair<iterator, bool>(iterator(__ndptr, this), __inserted);
#else
    return std::pair<iterator, bool>(iterator(__ndptr), __inserted);
#endif
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__node_insert_multi(__node_pointer __cp)
{
    __cp->__hash_ = hash_function()(__cp->__value_);
    size_type __bc = bucket_count();
    if (size()+1 > __bc * max_load_factor() || __bc == 0)
    {
        rehash(std::max<size_type>(2 * __bc + !__is_hash_power2(__bc),
                       size_type(ceil(float(size() + 1) / max_load_factor()))));
        __bc = bucket_count();
    }
    size_t __chash = __constrain_hash(__cp->__hash_, __bc);
    __node_pointer __pn = __bucket_list_[__chash];
    if (__pn == nullptr)
    {
        __pn = static_cast<__node_pointer>(std::pointer_traits<__node_base_pointer>::pointer_to(std::get<0>(__p1_)));
        __cp->__next_ = __pn->__next_;
        __pn->__next_ = __cp;
        // fix up __bucket_list_
        __bucket_list_[__chash] = __pn;
        if (__cp->__next_ != nullptr)
            __bucket_list_[__constrain_hash(__cp->__next_->__hash_, __bc)] = __cp;
    }
    else
    {
        for (bool __found = false; __pn->__next_ != nullptr &&
                                   __constrain_hash(__pn->__next_->__hash_, __bc) == __chash;
                                                           __pn = __pn->__next_)
        {
            //      __found    key_eq()     action
            //      false       false       loop
            //      true        true        loop
            //      false       true        set __found to true
            //      true        false       break
            if (__found != (__pn->__next_->__hash_ == __cp->__hash_ &&
                            key_eq()(__pn->__next_->__value_, __cp->__value_)))
            {
                if (!__found)
                    __found = true;
                else
                    break;
            }
        }
        __cp->__next_ = __pn->__next_;
        __pn->__next_ = __cp;
        if (__cp->__next_ != nullptr)
        {
            size_t __nhash = __constrain_hash(__cp->__next_->__hash_, __bc);
            if (__nhash != __chash)
                __bucket_list_[__nhash] = __cp;
        }
    }
    ++size();
#if _LIBCPP_DEBUG_LEVEL >= 2
    return iterator(__cp, this);
#else
    return iterator(__cp);
#endif
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__node_insert_multi(
        const_iterator __p, __node_pointer __cp)
{
#if _LIBCPP_DEBUG_LEVEL >= 2
    _LIBCPP_ASSERT(__get_const_db()->__find_c_from_i(&__p) == this,
        "unordered container::emplace_hint(const_iterator, args...) called with an iterator not"
        " referring to this unordered container");
#endif
    if (__p != end() && key_eq()(*__p, __cp->__value_))
    {
        __node_pointer __np = __p.__node_;
        __cp->__hash_ = __np->__hash_;
        size_type __bc = bucket_count();
        if (size()+1 > __bc * max_load_factor() || __bc == 0)
        {
            rehash(std::max<size_type>(2 * __bc + !__is_hash_power2(__bc),
                           size_type(ceil(float(size() + 1) / max_load_factor()))));
            __bc = bucket_count();
        }
        size_t __chash = __constrain_hash(__cp->__hash_, __bc);
        __node_pointer __pp = __bucket_list_[__chash];
        while (__pp->__next_ != __np)
            __pp = __pp->__next_;
        __cp->__next_ = __np;
        __pp->__next_ = __cp;
        ++size();
#if _LIBCPP_DEBUG_LEVEL >= 2
        return iterator(__cp, this);
#else
        return iterator(__cp);
#endif
    }
    return __node_insert_multi(__cp);
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
std::pair<typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator, bool>
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__insert_unique(const value_type& __x)
{
    return __insert_unique_value(__x);
}


#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class _ValueTp>

std::pair<typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator, bool>
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__insert_unique_value(_ValueTp&& __x)
#else
template <class _Tp, class _Hash, class _Equal, class _Alloc>

std::pair<typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator, bool>
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__insert_unique_value(const value_type& __x)
#endif
{
#if defined(_LIBCPP_HAS_NO_RVALUE_REFERENCES)
    typedef const value_type& _ValueTp;
#endif
    size_t __hash = hash_function()(__x);
    size_type __bc = bucket_count();
    bool __inserted = false;
    __node_pointer __nd;
    size_t __chash;
    if (__bc != 0)
    {
        __chash = __constrain_hash(__hash, __bc);
        __nd = __bucket_list_[__chash];
        if (__nd != nullptr)
        {
            for (__nd = __nd->__next_; __nd != nullptr &&
                                       __constrain_hash(__nd->__hash_, __bc) == __chash;
                                                           __nd = __nd->__next_)
            {
                if (key_eq()(__nd->__value_, __x))
                    goto __done;
            }
        }
    }
    {
        __node_holder __h = __construct_node(std::forward<_ValueTp>(__x), __hash);
        if (size()+1 > __bc * max_load_factor() || __bc == 0)
        {
            rehash(std::max<size_type>(2 * __bc + !__is_hash_power2(__bc),
                           size_type(ceil(float(size() + 1) / max_load_factor()))));
            __bc = bucket_count();
            __chash = __constrain_hash(__hash, __bc);
        }
        // insert_after __bucket_list_[__chash], or __first_node if bucket is null
        __node_pointer __pn = __bucket_list_[__chash];
        if (__pn == nullptr)
        {
            __pn = static_cast<__node_pointer>(std::pointer_traits<__node_base_pointer>::pointer_to(std::get<0>(__p1_)));
            __h->__next_ = __pn->__next_;
            __pn->__next_ = __h.get();
            // fix up __bucket_list_
            __bucket_list_[__chash] = __pn;
            if (__h->__next_ != nullptr)
                __bucket_list_[__constrain_hash(__h->__next_->__hash_, __bc)] = __h.get();
        }
        else
        {
            __h->__next_ = __pn->__next_;
            __pn->__next_ = __h.get();
        }
        __nd = __h.release();
        // increment size
        ++size();
        __inserted = true;
    }
__done:
#if _LIBCPP_DEBUG_LEVEL >= 2
    return std::pair<iterator, bool>(iterator(__nd, this), __inserted);
#else
    return std::pair<iterator, bool>(iterator(__nd), __inserted);
#endif
}

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
#ifndef _LIBCPP_HAS_NO_VARIADICS

template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class... _Args>
std::pair<typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator, bool>
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__emplace_unique(_Args&&... __args)
{
    __node_holder __h = __construct_node(std::forward<_Args>(__args)...);
    std::pair<iterator, bool> __r = __node_insert_unique(__h.get());
    if (__r.second)
        __h.release();
    return __r;
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class... _Args>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__emplace_multi(_Args&&... __args)
{
    __node_holder __h = __construct_node(std::forward<_Args>(__args)...);
    iterator __r = __node_insert_multi(__h.get());
    __h.release();
    return __r;
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class... _Args>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__emplace_hint_multi(
        const_iterator __p, _Args&&... __args)
{
#if _LIBCPP_DEBUG_LEVEL >= 2
    _LIBCPP_ASSERT(__get_const_db()->__find_c_from_i(&__p) == this,
        "unordered container::emplace_hint(const_iterator, args...) called with an iterator not"
        " referring to this unordered container");
#endif
    __node_holder __h = __construct_node(std::forward<_Args>(__args)...);
    iterator __r = __node_insert_multi(__p, __h.get());
    __h.release();
    return __r;
}

#endif  // _LIBCPP_HAS_NO_VARIADICS

template <class _Tp, class _Hash, class _Equal, class _Alloc>
std::pair<typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator, bool>
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__insert_unique(value_type&& __x)
{
    return __insert_unique_value(std::move(__x));
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class _Pp>
std::pair<typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator, bool>
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__insert_unique(_Pp&& __x)
{
    __node_holder __h = __construct_node(std::forward<_Pp>(__x));
    std::pair<iterator, bool> __r = __node_insert_unique(__h.get());
    if (__r.second)
        __h.release();
    return __r;
}

#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class _Pp>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__insert_multi(_Pp&& __x)
{
    __node_holder __h = __construct_node(std::forward<_Pp>(__x));
    iterator __r = __node_insert_multi(__h.get());
    __h.release();
    return __r;
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class _Pp>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__insert_multi(const_iterator __p,
                                                         _Pp&& __x)
{
#if _LIBCPP_DEBUG_LEVEL >= 2
    _LIBCPP_ASSERT(__get_const_db()->__find_c_from_i(&__p) == this,
        "unordered container::insert(const_iterator, rvalue) called with an iterator not"
        " referring to this unordered container");
#endif
    __node_holder __h = __construct_node(std::forward<_Pp>(__x));
    iterator __r = __node_insert_multi(__p, __h.get());
    __h.release();
    return __r;
}

#else  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _Tp, class _Hash, class _Equal, class _Alloc>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__insert_multi(const value_type& __x)
{
    __node_holder __h = __construct_node(__x);
    iterator __r = __node_insert_multi(__h.get());
    __h.release();
    return __r;
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__insert_multi(const_iterator __p,
                                                         const value_type& __x)
{
#if _LIBCPP_DEBUG_LEVEL >= 2
    _LIBCPP_ASSERT(__get_const_db()->__find_c_from_i(&__p) == this,
        "unordered container::insert(const_iterator, lvalue) called with an iterator not"
        " referring to this unordered container");
#endif
    __node_holder __h = __construct_node(__x);
    iterator __r = __node_insert_multi(__p, __h.get());
    __h.release();
    return __r;
}

#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _Tp, class _Hash, class _Equal, class _Alloc>
void
__hash_table<_Tp, _Hash, _Equal, _Alloc>::rehash(size_type __n)
{
    if (__n == 1)
        __n = 2;
    else if (__n & (__n - 1))
        __n = __next_prime(__n);
    size_type __bc = bucket_count();
    if (__n > __bc)
        __rehash(__n);
    else if (__n < __bc)
    {
        __n = std::max<size_type>
              (
                  __n,
                  __is_hash_power2(__bc) ? __next_hash_pow2(size_t(ceil(float(size()) / max_load_factor()))) :
                                           __next_prime(size_t(ceil(float(size()) / max_load_factor())))
              );
        if (__n < __bc)
            __rehash(__n);
    }
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
void
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__rehash(size_type __nbc)
{
#if _LIBCPP_DEBUG_LEVEL >= 2
    __get_db()->__invalidate_all(this);
#endif  // _LIBCPP_DEBUG_LEVEL >= 2
    __pointer_allocator& __npa = __bucket_list_.get_deleter().__alloc();
    __bucket_list_.reset(__nbc > 0 ?
                      __pointer_alloc_traits::allocate(__npa, __nbc) : nullptr);
    __bucket_list_.get_deleter().size() = __nbc;
    if (__nbc > 0)
    {
        for (size_type __i = 0; __i < __nbc; ++__i)
            __bucket_list_[__i] = nullptr;
        __node_pointer __pp(static_cast<__node_pointer>(std::pointer_traits<__node_base_pointer>::pointer_to(std::get<0>(__p1_))));
        __node_pointer __cp = __pp->__next_;
        if (__cp != nullptr)
        {
            size_type __chash = __constrain_hash(__cp->__hash_, __nbc);
            __bucket_list_[__chash] = __pp;
            size_type __phash = __chash;
            for (__pp = __cp, __cp = __cp->__next_; __cp != nullptr;
                                                           __cp = __pp->__next_)
            {
                __chash = __constrain_hash(__cp->__hash_, __nbc);
                if (__chash == __phash)
                    __pp = __cp;
                else
                {
                    if (__bucket_list_[__chash] == nullptr)
                    {
                        __bucket_list_[__chash] = __pp;
                        __pp = __cp;
                        __phash = __chash;
                    }
                    else
                    {
                        __node_pointer __np = __cp;
                        for (; __np->__next_ != nullptr &&
                               key_eq()(__cp->__value_, __np->__next_->__value_);
                                                           __np = __np->__next_)
                            ;
                        __pp->__next_ = __np->__next_;
                        __np->__next_ = __bucket_list_[__chash]->__next_;
                        __bucket_list_[__chash]->__next_ = __cp;

                    }
                }
            }
        }
    }
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class _Key>
inline typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator
__hash_table<_Tp, _Hash, _Equal, _Alloc>::find(const _Key& __k)
{
    size_t __hash = hash_function()(__k);
    size_type __bc = bucket_count();
    if (__bc != 0)
    {
        size_t __chash = __constrain_hash(__hash, __bc);
        __node_pointer __nd = __bucket_list_[__chash];
        if (__nd != nullptr)
        {
            for (__nd = __nd->__next_; __nd != nullptr &&
                                       __constrain_hash(__nd->__hash_, __bc) == __chash;
                                                           __nd = __nd->__next_)
            {
                if (key_eq()(__nd->__value_, __k))
#if _LIBCPP_DEBUG_LEVEL >= 2
                    return iterator(__nd, this);
#else
                    return iterator(__nd);
#endif
            }
        }
    }
    return end();
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class _Key>
inline typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::const_iterator
__hash_table<_Tp, _Hash, _Equal, _Alloc>::find(const _Key& __k) const
{
    size_t __hash = hash_function()(__k);
    size_type __bc = bucket_count();
    if (__bc != 0)
    {
        size_t __chash = __constrain_hash(__hash, __bc);
        __node_const_pointer __nd = __bucket_list_[__chash];
        if (__nd != nullptr)
        {
            for (__nd = __nd->__next_; __nd != nullptr &&
                                           __constrain_hash(__nd->__hash_, __bc) == __chash;
                                                           __nd = __nd->__next_)
            {
                if (key_eq()(__nd->__value_, __k))
#if _LIBCPP_DEBUG_LEVEL >= 2
                    return const_iterator(__nd, this);
#else
                    return const_iterator(__nd);
#endif
            }
        }

    }
    return end();
}

#ifndef _LIBCPP_HAS_NO_RVALUE_REFERENCES
#ifndef _LIBCPP_HAS_NO_VARIADICS

template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class ..._Args>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::__node_holder
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__construct_node(_Args&& ...__args)
{
    __node_allocator& __na = __node_alloc();
    __node_holder __h(__node_traits::allocate(__na, 1), _Dp(__na));
    __node_traits::construct(__na, std::addressof(__h->__value_), std::forward<_Args>(__args)...);
    __h.get_deleter().__value_constructed = true;
    __h->__hash_ = hash_function()(__h->__value_);
    __h->__next_ = nullptr;
    return __h;
}

#endif  // _LIBCPP_HAS_NO_VARIADICS

template <class _Tp, class _Hash, class _Equal, class _Alloc>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::__node_holder
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__construct_node(value_type&& __v,
                                                           size_t __hash)
{
    __node_allocator& __na = __node_alloc();
    __node_holder __h(__node_traits::allocate(__na, 1), _Dp(__na));
    __node_traits::construct(__na, std::addressof(__h->__value_), std::move(__v));
    __h.get_deleter().__value_constructed = true;
    __h->__hash_ = __hash;
    __h->__next_ = nullptr;
    return __h;
}

#else  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _Tp, class _Hash, class _Equal, class _Alloc>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::__node_holder
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__construct_node(const value_type& __v)
{
    __node_allocator& __na = __node_alloc();
    __node_holder __h(__node_traits::allocate(__na, 1), _Dp(__na));
    __node_traits::construct(__na, std::addressof(__h->__value_), __v);
    __h.get_deleter().__value_constructed = true;
    __h->__hash_ = hash_function()(__h->__value_);
    __h->__next_ = nullptr;
    return std::move(__h);  // explicitly moved for C++03
}

#endif  // _LIBCPP_HAS_NO_RVALUE_REFERENCES

template <class _Tp, class _Hash, class _Equal, class _Alloc>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::__node_holder
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__construct_node(const value_type& __v,
                                                           size_t __hash)
{
    __node_allocator& __na = __node_alloc();
    __node_holder __h(__node_traits::allocate(__na, 1), _Dp(__na));
    __node_traits::construct(__na, std::addressof(__h->__value_), __v);
    __h.get_deleter().__value_constructed = true;
    __h->__hash_ = __hash;
    __h->__next_ = nullptr;
    return std::move(__h);  // explicitly moved for C++03
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator
__hash_table<_Tp, _Hash, _Equal, _Alloc>::erase(const_iterator __p)
{
    __node_pointer __np = __p.__node_;
#if _LIBCPP_DEBUG_LEVEL >= 2
    _LIBCPP_ASSERT(__get_const_db()->__find_c_from_i(&__p) == this,
        "unordered container erase(iterator) called with an iterator not"
        " referring to this container");
    _LIBCPP_ASSERT(__p != end(),
        "unordered container erase(iterator) called with a non-dereferenceable iterator");
    iterator __r(__np, this);
#else
    iterator __r(__np);
#endif
    ++__r;
    remove(__p);
    return __r;
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator
__hash_table<_Tp, _Hash, _Equal, _Alloc>::erase(const_iterator __first,
                                                const_iterator __last)
{
#if _LIBCPP_DEBUG_LEVEL >= 2
    _LIBCPP_ASSERT(__get_const_db()->__find_c_from_i(&__first) == this,
        "unodered container::erase(iterator, iterator) called with an iterator not"
        " referring to this unodered container");
    _LIBCPP_ASSERT(__get_const_db()->__find_c_from_i(&__last) == this,
        "unodered container::erase(iterator, iterator) called with an iterator not"
        " referring to this unodered container");
#endif
    for (const_iterator __p = __first; __first != __last; __p = __first)
    {
        ++__first;
        erase(__p);
    }
    __node_pointer __np = __last.__node_;
#if _LIBCPP_DEBUG_LEVEL >= 2
    return iterator (__np, this);
#else
    return iterator (__np);
#endif
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class _Key>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::size_type
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__erase_unique(const _Key& __k)
{
    iterator __i = find(__k);
    if (__i == end())
        return 0;
    erase(__i);
    return 1;
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class _Key>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::size_type
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__erase_multi(const _Key& __k)
{
    size_type __r = 0;
    iterator __i = find(__k);
    if (__i != end())
    {
        iterator __e = end();
        do
        {
            erase(__i++);
            ++__r;
        } while (__i != __e && key_eq()(*__i, __k));
    }
    return __r;
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::__node_holder
__hash_table<_Tp, _Hash, _Equal, _Alloc>::remove(const_iterator __p) noexcept
{
    // current node
    __node_pointer __cn = __p.__node_;
    size_type __bc = bucket_count();
    size_t __chash = __constrain_hash(__cn->__hash_, __bc);
    // find previous node
    __node_pointer __pn = __bucket_list_[__chash];
    for (; __pn->__next_ != __cn; __pn = __pn->__next_)
        ;
    // Fix up __bucket_list_
        // if __pn is not in same bucket (before begin is not in same bucket) &&
        //    if __cn->__next_ is not in same bucket (nullptr is not in same bucket)
    if (__pn == static_cast<__node_pointer>(std::pointer_traits<__node_base_pointer>::pointer_to(std::get<0>(__p1_)))
                            || __constrain_hash(__pn->__hash_, __bc) != __chash)
    {
        if (__cn->__next_ == nullptr || __constrain_hash(__cn->__next_->__hash_, __bc) != __chash)
            __bucket_list_[__chash] = nullptr;
    }
        // if __cn->__next_ is not in same bucket (nullptr is in same bucket)
    if (__cn->__next_ != nullptr)
    {
        size_t __nhash = __constrain_hash(__cn->__next_->__hash_, __bc);
        if (__nhash != __chash)
            __bucket_list_[__nhash] = __pn;
    }
    // remove __cn
    __pn->__next_ = __cn->__next_;
    __cn->__next_ = nullptr;
    --size();
#if _LIBCPP_DEBUG_LEVEL >= 2
    __c_node* __c = __get_db()->__find_c_and_lock(this);
    for (__i_node** __p = __c->end_; __p != __c->beg_; )
    {
        --__p;
        iterator* __i = static_cast<iterator*>((*__p)->__i_);
        if (__i->__node_ == __cn)
        {
            (*__p)->__c_ = nullptr;
            if (--__c->end_ != __p)
                memmove(__p, __p+1, (__c->end_ - __p)*sizeof(__i_node*));
        }
    }
    __get_db()->unlock();
#endif
    return __node_holder(__cn, _Dp(__node_alloc(), true));
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class _Key>
inline
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::size_type
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__count_unique(const _Key& __k) const
{
    return static_cast<size_type>(find(__k) != end());
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class _Key>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::size_type
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__count_multi(const _Key& __k) const
{
    size_type __r = 0;
    const_iterator __i = find(__k);
    if (__i != end())
    {
        const_iterator __e = end();
        do
        {
            ++__i;
            ++__r;
        } while (__i != __e && key_eq()(*__i, __k));
    }
    return __r;
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class _Key>
std::pair<typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator,
     typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator>
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__equal_range_unique(
        const _Key& __k)
{
    iterator __i = find(__k);
    iterator __j = __i;
    if (__i != end())
        ++__j;
    return std::pair<iterator, iterator>(__i, __j);
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class _Key>
std::pair<typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::const_iterator,
     typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::const_iterator>
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__equal_range_unique(
        const _Key& __k) const
{
    const_iterator __i = find(__k);
    const_iterator __j = __i;
    if (__i != end())
        ++__j;
    return std::pair<const_iterator, const_iterator>(__i, __j);
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class _Key>
std::pair<typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator,
     typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::iterator>
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__equal_range_multi(
        const _Key& __k)
{
    iterator __i = find(__k);
    iterator __j = __i;
    if (__i != end())
    {
        iterator __e = end();
        do
        {
            ++__j;
        } while (__j != __e && key_eq()(*__j, __k));
    }
    return std::pair<iterator, iterator>(__i, __j);
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
template <class _Key>
std::pair<typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::const_iterator,
     typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::const_iterator>
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__equal_range_multi(
        const _Key& __k) const
{
    const_iterator __i = find(__k);
    const_iterator __j = __i;
    if (__i != end())
    {
        const_iterator __e = end();
        do
        {
            ++__j;
        } while (__j != __e && key_eq()(*__j, __k));
    }
    return std::pair<const_iterator, const_iterator>(__i, __j);
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
void
__hash_table<_Tp, _Hash, _Equal, _Alloc>::swap(__hash_table& __u)
    noexcept
{
    {
    __node_pointer_pointer __npp = __bucket_list_.release();
    __bucket_list_.reset(__u.__bucket_list_.release());
    __u.__bucket_list_.reset(__npp);
    }
    std::swap(__bucket_list_.get_deleter().size(), __u.__bucket_list_.get_deleter().size());
    __swap_allocator(__bucket_list_.get_deleter().__alloc(),
             __u.__bucket_list_.get_deleter().__alloc());
    __swap_allocator(__node_alloc(), __u.__node_alloc());
    std::swap(std::get<0>(__p1_).__next_, std::get<0>(__u.__p1_).__next_);
    __p2_.swap(__u.__p2_);
    __p3_.swap(__u.__p3_);
    if (size() > 0)
        __bucket_list_[__constrain_hash(std::get<0>(__p1_).__next_->__hash_, bucket_count())] =
            static_cast<__node_pointer>(std::pointer_traits<__node_base_pointer>::pointer_to(std::get<0>(__p1_)));
    if (__u.size() > 0)
        __u.__bucket_list_[__constrain_hash(std::get<0>(__u.__p1_).__next_->__hash_, __u.bucket_count())] =
            static_cast<__node_pointer>(std::pointer_traits<__node_base_pointer>::pointer_to(std::get<0>(__u.__p1_)));
#if _LIBCPP_DEBUG_LEVEL >= 2
    __get_db()->swap(this, &__u);
#endif
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
typename __hash_table<_Tp, _Hash, _Equal, _Alloc>::size_type
__hash_table<_Tp, _Hash, _Equal, _Alloc>::bucket_size(size_type __n) const
{
    _LIBCPP_ASSERT(__n < bucket_count(),
        "unordered container::bucket_size(n) called with n >= bucket_count()");
    __node_const_pointer __np = __bucket_list_[__n];
    size_type __bc = bucket_count();
    size_type __r = 0;
    if (__np != nullptr)
    {
        for (__np = __np->__next_; __np != nullptr &&
                                   __constrain_hash(__np->__hash_, __bc) == __n;
                                                    __np = __np->__next_, ++__r)
            ;
    }
    return __r;
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
inline
void
swap(__hash_table<_Tp, _Hash, _Equal, _Alloc>& __x,
     __hash_table<_Tp, _Hash, _Equal, _Alloc>& __y)
    noexcept(noexcept(__x.swap(__y)))
{
    __x.swap(__y);
}

#if _LIBCPP_DEBUG_LEVEL >= 2

template <class _Tp, class _Hash, class _Equal, class _Alloc>
bool
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__dereferenceable(const const_iterator* __i) const
{
    return __i->__node_ != nullptr;
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
bool
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__decrementable(const const_iterator*) const
{
    return false;
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
bool
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__addable(const const_iterator*, ptrdiff_t) const
{
    return false;
}

template <class _Tp, class _Hash, class _Equal, class _Alloc>
bool
__hash_table<_Tp, _Hash, _Equal, _Alloc>::__subscriptable(const const_iterator*, ptrdiff_t) const
{
    return false;
}

#endif  // _LIBCPP_DEBUG_LEVEL >= 2
}

#endif  // _LIBCPP__HASH_TABLE
