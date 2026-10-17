// Единый интерфейс «номера» для шаблонных алгоритмов.
//
// Алгоритмы rank/select пишутся один раз как шаблоны от типа номера Num:
//   std::uint64_t — быстро, но только пока номер помещается в 64 бита;
//   combi::BigUInt — без ограничений, но каждая операция стоит O(длины числа).
// Алгоритмам нужны только «короткие» операции: умножить/прибавить/поделить
// на число до 2^32.
#pragma once

#include <cstdint>
#include <stdexcept>

#include "biguint.hpp"

namespace combi {

template <class Num>
struct NumTraits;

template <>
struct NumTraits<std::uint64_t> {
    static constexpr const char* name = "uint64";
    // 20! = 2 432 902 008 176 640 000 < 2^64 < 21!
    static constexpr std::uint32_t max_perm_n = 20;
};

template <>
struct NumTraits<BigUInt> {
    static constexpr const char* name = "BigUInt";
    static constexpr std::uint32_t max_perm_n = 0xFFFFFFFFu;
};

namespace num {

inline void mul_small(std::uint64_t& x, std::uint32_t m) { x *= m; }
inline void add_small(std::uint64_t& x, std::uint32_t a) { x += a; }
inline std::uint32_t divmod_small(std::uint64_t& x, std::uint32_t m) {
    const auto r = static_cast<std::uint32_t>(x % m);
    x /= m;
    return r;
}
inline bool is_zero(std::uint64_t x) { return x == 0; }
inline std::uint32_t low_bits(std::uint64_t x) { return static_cast<std::uint32_t>(x); }

inline void mul_small(BigUInt& x, std::uint32_t m) { x.mul_small(m); }
inline void add_small(BigUInt& x, std::uint32_t a) { x.add_small(a); }
inline std::uint32_t divmod_small(BigUInt& x, std::uint32_t m) { return x.divmod_small(m); }
inline bool is_zero(const BigUInt& x) { return x.is_zero(); }
inline std::uint32_t low_bits(const BigUInt& x) { return x.low_limb(); }

}  // namespace num

// n! в выбранном типе (для uint64_t — только n ≤ 20).
template <class Num>
Num factorial(std::uint32_t n) {
    if (n > NumTraits<Num>::max_perm_n) throw std::overflow_error("factorial: n! не помещается в выбранный тип");
    Num r = 1;
    for (std::uint32_t i = 2; i <= n; ++i) num::mul_small(r, i);
    return r;
}

}  // namespace combi
