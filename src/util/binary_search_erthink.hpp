// https://gitflic.ru/project/erthink/bsearch-try
#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#ifndef __has_attribute
#define __has_attribute(x) (0)
#endif /* __has_attribute */
#ifndef __has_builtin
#define __has_builtin(x) (0)
#endif /* __has_builtin */
#ifndef __fallthrough
#if __has_attribute(__fallthrough__)
#define __fallthrough __attribute__((__fallthrough__))
#else
#define __fallthrough
#endif
#endif /* __fallthrough */
#if defined(__INTEL_COMPILER)
#define __builtin_expect_with_probability(expr, value, prob) (expr)
#endif
#if !defined(__builtin_expect_with_probability) &&                             \
    !__has_builtin(__builtin_expect_with_probability) &&                       \
    (!defined(__GNUC__) || __GNUC__ < 9 || defined(__INTEL_COMPILER))
#define __builtin_expect_with_probability(expr, value, prob) (expr)
#endif /* __builtin_expect_with_probability */
#if defined(__clang__) && __clang_major__ > 4 &&                               \
    (defined(i386) || defined(__386) || defined(__i386) ||                     \
     defined(__i386__) || defined(i486) || defined(__i486) ||                  \
     defined(__i486__) || defined(i586) | defined(__i586) ||                   \
     defined(__i586__) || defined(i686) || defined(__i686) ||                  \
     defined(__i686__) || defined(_M_IX86) || defined(_X86_) ||                \
     defined(__THW_INTEL__) || defined(__I86__) || defined(__INTEL__) ||       \
     defined(__x86_64) || defined(__x86_64__) || defined(__amd64__) ||         \
     defined(__amd64) || defined(_M_X64) || defined(_M_AMD64) ||               \
     defined(__IA32__) || defined(__INTEL__))
#define WORKAROUND_FOR_CLANG_OPTIMIZER_BUG(size, flag)                         \
  do                                                                           \
    __asm __volatile(""                                                        \
                     : "+r"(size)                                              \
                     : "r" /* the `b` constraint is more suitable here, but    \
                              cause CLANG to allocate and push/pop an one more \
                              register, so using the `r` which avoids this. */ \
                     (flag));                                                  \
  while (0)
#else
#define WORKAROUND_FOR_CLANG_OPTIMIZER_BUG(size, flag)                         \
  do                                                                           \
  {/* nope for non-clang or non-x86 */}                                        \
  while (0)
#endif /* Workaroind for CLANG */
/* Адаптивно-упрощенный шаг двоичного поиска:
 *  - без переходов при наличии cmov или аналога;
 *  - допускает лишние итерации;
 *  - но ищет пока size > 2, что требует дозавершения поиска
 *    среди остающихся 0-1-2 элементов. */
#define BINARY_SEARCH_STEP                                                     \
  do {                                                                         \
    const TYPE *const middle = it + (size >> 1);                               \
    size = (size + 1) >> 1;                                                    \
    const bool flag = CMP(*middle, key);                                       \
    WORKAROUND_FOR_CLANG_OPTIMIZER_BUG(size, flag);                            \
    it = flag ? middle : it;                                                   \
  } while (0)
#if 0
/* Компромиссный вариант для альтернативно-одаренного оптимизатора clang.
 *
 * Основное отличие в том, что при "не равно" (true от компаратора) переход
 * делается на 1 ближе к концу массива. Алгоритмически это верно и обеспечивает
 * чуть-чуть более быструю сходимость, но зато требует больше вычислений.
 * Поэтому в целом это более медленно в сравнении с базовым вариантом,
 * а также ВАЖНО(!) не допускается спекулятивное выполнение при size == 0.
 *
 * Однако, двойная зависимость от flag (видимо из-за ошибки в оптимизаторе)
 * подталкивает clang >= 5 к использованию cmov вместо условных переходов. */
#define BINARY_SEARCH_STEP                                                     \
  do {                                                                         \
    const TYPE *const middle = it + (size >> 1);                               \
    const bool flag = CMP(*middle, key);                                       \
    size = ((size + 1) >> 1) - flag;                                           \
    it = flag ? middle + 1 : it;                                               \
  } while (0)
#endif /* #if 0 */
#define CMP(a, b) __builtin_expect_with_probability((a) < (b), 0, 0.5)
//------------------------------------------------------------------------------
template<typename TYPE, typename VAL, typename CMP>
const TYPE *ly_bl_mini0(const TYPE *it, size_t size, const VAL& key, CMP && cmp){
  /* Минималистичный вариант с предусловием:
   *  - без обращений за границы при size == 0;
   *  - "совместим" с багом clang;
   *  - сходимость 2:1 на итерацию цикла;
   *  - поиск среди до 4 элементов включительно без повторов цикла. */
  while (size > 2)
  {
      const TYPE *const middle = it + (size >> 1);                               \
      size = (size + 1) >> 1;                                                    \
      const bool flag = cmp(*middle, key);                                       \
      WORKAROUND_FOR_CLANG_OPTIMIZER_BUG(size, flag);                            \
      it = flag ? middle : it;                                                   \
  }
  it += size > 1 && cmp(*it, key);
  it += size > 0 && cmp(*it, key);
  return it;
}
template<typename TYPE, typename VAL>
const TYPE *ly_bl_mini0(const TYPE *it, size_t size, const VAL& key){
    return ly_bl_mini0(it, size, key, std::less<>{});
}

template<typename TYPE, typename VAL>
const TYPE *ly_bl_mini1(const TYPE *it, size_t size, const VAL& key){
  /* Минималистичный вариант с постусловием:
   *  - компилятор/процессор имеют возможность заранее готовиться к переходу;
   *  - сходимость 2:1 на итерацию цикла;
   *  - поиск среди до 4 элементов включительно без повторов цикла;
   *  - в худшем случае один лишний BINARY_SEARCH_STEP. */
  do {
    BINARY_SEARCH_STEP;
  } while (size > 2);
  it += size > 1 && CMP(*it, key);
  it += size > 0 && CMP(*it, key);
  return it;
}
template<typename TYPE, typename VAL>
const TYPE *ly_bl_mini2(const TYPE *it, size_t size, const VAL& key){
  /* Минималистичный вариант с хорошим балансом:
   *  - компилятор/процессор имеют возможность заранее готовиться к переходу;
   *  - сходимость 4:1 на итерацию цикла;
   *  - поиск среди до 8 элементов включительно без повторов цикла;
   *  - в худшем случае два лишних BINARY_SEARCH_STEP. */
  do {
    BINARY_SEARCH_STEP;
    BINARY_SEARCH_STEP;
  } while (size > 2);
  it += size > 1 && CMP(*it, key);
  it += size > 0 && CMP(*it, key);
  return it;
}
template<typename TYPE, typename VAL>
const TYPE *ly_bl_mini3(const TYPE *it, size_t size, const VAL& key){
  /* Жадный минималистичный вариант с хорошим балансом:
   *  - компилятор/процессор имеют возможность заранее готовиться к переходу;
   *  - сходимость 8:1 на итерацию цикла;
   *  - поиск среди до 16 элементов включительно без повторов цикла;
   *  - в худшем случае три лишних BINARY_SEARCH_STEP. */
  do {
    BINARY_SEARCH_STEP;
    BINARY_SEARCH_STEP;
    BINARY_SEARCH_STEP;
  } while (size > 2);
  it += size > 1 && CMP(*it, key);
  it += size > 0 && CMP(*it, key);
  return it;
}
template<typename TYPE, typename VAL>
const TYPE *ly_bl_switch1(const TYPE *it, size_t size, const VAL& key){
  /* Расширенный вариант, с расчётом на использование компилятором таблицы
   * переходов для реализации оператора switch. Если же компилятор поступает
   * как-то иначе, то этот вариант быстро теряет преимущества перед
   * минималистичным. В том числе поэтому, для стимулирования использования
   * таблицы переходов, добавлена обработка значений 5-6-7-8.
   *  - компилятор/процессор имеют возможность заранее готовиться к переходу;
   *  - сходимость 16:1 на итерацию цикла;
   *  - цикл и выход всегда через косвенный переход;
   *  - поиск среди до 32 элементов включительно без повторов цикла;
   *  - в худшем случае два лишних BINARY_SEARCH_STEP;
   *  - можно убрать по-одному BINARY_SEARCH_STEP или добавить еще несколько. */
  while (1) {
    BINARY_SEARCH_STEP;
    switch (size) {
    default:
      BINARY_SEARCH_STEP;
      continue;
    case 8:
    case 7:
    case 6:
    case 5:
      BINARY_SEARCH_STEP;
      __fallthrough /* fall through */;
    case 4:
    case 3:
      BINARY_SEARCH_STEP;
      __fallthrough /* fall through */;
    case 2:
      it += CMP(*it, key);
      __fallthrough /* fall through */;
    case 1:
      it += CMP(*it, key);
      __fallthrough /* fall through */;
    case 0:
      return it;
    }
  }
}
template<typename TYPE, typename VAL>
const TYPE *ly_bl_switch2(const TYPE *it, size_t size, const VAL& key){
  /* Расширенный вариант, с расчётом на использование компилятором таблицы
   * переходов для реализации оператора switch. Если же компилятор поступает
   * как-то иначе, то этот вариант быстро теряет преимущества перед
   * минималистичным. В том числе поэтому, для стимулирования использования
   * таблицы переходов, добавлена обработка значений 5-6-7-8.
   *  - компилятор/процессор имеют возможность заранее готовиться к переходу;
   *  - сходимость 16:1 на итерацию цикла;
   *  - цикл и выход всегда через косвенный переход;
   *  - поиск среди до 32 элементов включительно без повторов цикла;
   *  - в худшем случае два лишних BINARY_SEARCH_STEP;
   *  - можно убрать по-одному BINARY_SEARCH_STEP или добавить еще несколько. */
  while (1) {
    BINARY_SEARCH_STEP;
    BINARY_SEARCH_STEP;
    switch (size) {
    default:
      BINARY_SEARCH_STEP;
      BINARY_SEARCH_STEP;
      continue;
    case 8:
    case 7:
    case 6:
    case 5:
      BINARY_SEARCH_STEP;
      __fallthrough /* fall through */;
    case 4:
    case 3:
      BINARY_SEARCH_STEP;
      __fallthrough /* fall through */;
    case 2:
      it += CMP(*it, key);
      __fallthrough /* fall through */;
    case 1:
      it += CMP(*it, key);
      __fallthrough /* fall through */;
    case 0:
      return it;
    }
  }
}
#ifdef __GNUC__
template<typename TYPE, typename VAL>
const TYPE *ly_bl_goto1(const TYPE *it, size_t size, const VAL& key){
  /* Расширенный вариант с явной таблицей переходов.
   *  - компилятор/процессор имеют возможность заранее готовиться к переходу;
   *  - сходимость 16:1 на итерацию цикла;
   *  - цикл и выход всегда через косвенный переход;
   *  - поиск среди до 32 элементов включительно без повторов цикла;
   *  - в худшем случае два лишних BINARY_SEARCH_STEP;
   *  - можно убрать по-одному BINARY_SEARCH_STEP или добавить еще несколько. */
  static const void *addr4jmp[] = {&&case_0,   &&case_1,   &&case_2,
                                   &&case_3_4, &&case_3_4, &&case_5_8,
                                   &&case_5_8, &&case_5_8, &&case_5_8};
  while (1) {
    const char will_jump = size < 17;
    const void *target = addr4jmp[((size + 1) >> 1) & 15];
    BINARY_SEARCH_STEP;
    if (will_jump)
      goto *target;
    BINARY_SEARCH_STEP;
  }
case_5_8:
  BINARY_SEARCH_STEP;
case_3_4:
  BINARY_SEARCH_STEP;
case_2:
  it += CMP(*it, key);
case_1:
  it += CMP(*it, key);
case_0:
  return it;
}
template<typename TYPE, typename VAL>
const TYPE *ly_bl_goto2(const TYPE *it, size_t size, const VAL& key){
  /* Расширенный вариант с явной таблицей переходов.
   *  - компилятор/процессор имеют возможность заранее готовиться к переходу;
   *  - сходимость 16:1 на итерацию цикла;
   *  - цикл и выход всегда через косвенный переход;
   *  - поиск среди до 32 элементов включительно без повторов цикла;
   *  - в худшем случае два лишних BINARY_SEARCH_STEP;
   *  - можно убрать по-одному BINARY_SEARCH_STEP или добавить еще несколько. */
  static const void *addr4jmp[] = {&&case_0,   &&case_1,   &&case_2,
                                   &&case_3_4, &&case_3_4, &&case_5_8,
                                   &&case_5_8, &&case_5_8, &&case_5_8};
  while (1) {
    // const char will_jump = size < 33;
    // const void *target = addr4jmp[((size + 3) >> 2) & 15];
    BINARY_SEARCH_STEP;
    const char will_jump = size < 17;
    const void *target = addr4jmp[((size + 1) >> 1) & 15];
    BINARY_SEARCH_STEP;
    if (will_jump)
      goto *target;
    BINARY_SEARCH_STEP;
    BINARY_SEARCH_STEP;
  }
case_5_8:
  BINARY_SEARCH_STEP;
case_3_4:
  BINARY_SEARCH_STEP;
case_2:
  it += CMP(*it, key);
case_1:
  it += CMP(*it, key);
case_0:
  return it;
}
#endif /* __GNUC__ */
/*----------------------------------------------------------------------------*/
#ifdef __GNUC__
template <typename T> inline unsigned clz(T v) noexcept;
template <> inline unsigned clz<unsigned>(unsigned v) noexcept {
  return __builtin_clz(v);
}
template <> inline unsigned clz<unsigned long>(unsigned long v) noexcept {
  return __builtin_clzl(v);
}
template <>
inline unsigned clz<unsigned long long>(unsigned long long v) noexcept {
  return __builtin_clzll(v);
}
inline unsigned nbits(size_t v) noexcept{
  return v ? sizeof(v) * __CHAR_BIT__ - clz(v) : 0;
}
template<typename TYPE, typename VAL>
const TYPE *ly_bl_clz_switch(const TYPE *it, size_t size, const VAL& key){
  switch (nbits(size)) {
  default:
    abort();
  case 31:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 30:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 29:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 28:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 27:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 26:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 25:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 24:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 23:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 22:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 21:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 20:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 19:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 18:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 17:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 16:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 15:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 14:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 13:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 12:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 11:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 10:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 9:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 8:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 7:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 6:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 5:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 4:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 3:
    BINARY_SEARCH_STEP;
    __fallthrough /* fall through */;
  case 2:
    BINARY_SEARCH_STEP;
    it += size > 1 && CMP(*it, key);
    __fallthrough /* fall through */;
  case 1:
    it += CMP(*it, key);
    __fallthrough /* fall through */;
  case 0:
    return it;
  }
}
template<typename TYPE, typename VAL>
const TYPE *ly_bl_clz_goto(const TYPE *it, size_t size, const VAL& key){
#define BITS 64
  static const void *addr4jmp[BITS] = {
#if BITS > 32
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
    &&oops,
#endif
    &&oops,
    &&case_31,
    &&case_30,
    &&case_29,
    &&case_28,
    &&case_27,
    &&case_26,
    &&case_25,
    &&case_24,
    &&case_23,
    &&case_22,
    &&case_21,
    &&case_20,
    &&case_19,
    &&case_18,
    &&case_17,
    &&case_16,
    &&case_15,
    &&case_14,
    &&case_13,
    &&case_12,
    &&case_11,
    &&case_10,
    &&case_9,
    &&case_8,
    &&case_7,
    &&case_6,
    &&case_5,
    &&case_4,
    &&case_3,
    &&case_2,
    &&case_1
  };
  size_t lz = clz(size);
  if (size)
    goto *addr4jmp[lz];
  return it;
oops:
  abort();
case_31:
  BINARY_SEARCH_STEP;
case_30:
  BINARY_SEARCH_STEP;
case_29:
  BINARY_SEARCH_STEP;
case_28:
  BINARY_SEARCH_STEP;
case_27:
  BINARY_SEARCH_STEP;
case_26:
  BINARY_SEARCH_STEP;
case_25:
  BINARY_SEARCH_STEP;
case_24:
  BINARY_SEARCH_STEP;
case_23:
  BINARY_SEARCH_STEP;
case_22:
  BINARY_SEARCH_STEP;
case_21:
  BINARY_SEARCH_STEP;
case_20:
  BINARY_SEARCH_STEP;
case_19:
  BINARY_SEARCH_STEP;
case_18:
  BINARY_SEARCH_STEP;
case_17:
  BINARY_SEARCH_STEP;
case_16:
  BINARY_SEARCH_STEP;
case_15:
  BINARY_SEARCH_STEP;
case_14:
  BINARY_SEARCH_STEP;
case_13:
  BINARY_SEARCH_STEP;
case_12:
  BINARY_SEARCH_STEP;
case_11:
  BINARY_SEARCH_STEP;
case_10:
  BINARY_SEARCH_STEP;
case_9:
  BINARY_SEARCH_STEP;
case_8:
  BINARY_SEARCH_STEP;
case_7:
  BINARY_SEARCH_STEP;
case_6:
  BINARY_SEARCH_STEP;
case_5:
  BINARY_SEARCH_STEP;
case_4:
  BINARY_SEARCH_STEP;
case_3:
  BINARY_SEARCH_STEP;
case_2:
  BINARY_SEARCH_STEP;
  it += size > 1 && CMP(*it, key);
case_1:
  it += CMP(*it, key);
  return it;
}
#endif /* __GNUC__ */
