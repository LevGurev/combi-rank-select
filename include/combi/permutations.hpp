// rank/select перестановок.
//
// Перестановка длины n — вектор p из чисел 0..n-1, каждое ровно один раз.
// rank(p)      — номер p в выбранном порядке (с нуля);
// select(r, n) — перестановка с номером r (в литературе — unrank).
//
// Лексикографический порядок (как в словаре): 012 < 021 < 102 < 120 < 201 < 210.
// Номер считается в два шага, и каждый шаг можно ускорять отдельно:
//   1) перестановка <-> код Лемера d: d[i] = сколько справа от p[i] чисел меньше p[i];
//   2) код Лемера <-> номер: r = Σ d[i]·(n-1-i)!  (факториальная система счисления).
//
// Что есть:
//   perm_rank_lex_naive / perm_unrank_lex_naive — по определению, O(n^2);
//   perm_rank_lex       / perm_unrank_lex       — с деревом Фенвика, O(n log n);
//   perm_rank_mr        / perm_unrank_mr        — Myrvold & Ruskey (2001), O(n),
//                                                 но порядок НЕ лексикографический.
//
// Num — тип номера: std::uint64_t (только n ≤ 20) или combi::BigUInt (любое n).
// Сложность указана в «коротких» операциях. С BigUInt каждая такая операция
// стоит O(длины номера) = O(n log n) бит, поэтому на больших n всё упирается
// в длинную арифметику. Это хорошо видно в замерах (bench) — и это одна из
// главных точек для «эффективной реализации».
#pragma once

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

#include "fenwick.hpp"
#include "num.hpp"

namespace combi {

using Perm = std::vector<std::uint32_t>;
using LehmerCode = std::vector<std::uint32_t>;

inline bool is_permutation(const Perm& p) {
    std::vector<bool> seen(p.size(), false);
    for (std::uint32_t v : p) {
        if (v >= p.size() || seen[v]) return false;
        seen[v] = true;
    }
    return true;
}

namespace detail {

inline void require_permutation(const Perm& p) {
    if (!is_permutation(p)) throw std::invalid_argument("ожидалась перестановка чисел 0..n-1");
}

template <class Num>
void require_perm_size(std::size_t n) {
    if (n > NumTraits<Num>::max_perm_n) {
        throw std::overflow_error("n! не помещается в выбранный тип номера; используйте BigUInt");
    }
}

}  // namespace detail

// ---------- Шаг 1: перестановка <-> код Лемера ----------

// O(n^2): для каждой позиции честно считаем меньшие элементы справа.
inline LehmerCode lehmer_code_naive(const Perm& p) {
    const std::size_t n = p.size();
    LehmerCode d(n, 0);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            if (p[j] < p[i]) ++d[i];
        }
    }
    return d;
}

// O(n log n): «меньшие справа» = «ещё не встреченные слева и меньшие p[i]».
inline LehmerCode lehmer_code_fenwick(const Perm& p) {
    const auto n = static_cast<std::uint32_t>(p.size());
    FenwickSet free_values(n);
    LehmerCode d(n);
    for (std::uint32_t i = 0; i < n; ++i) {
        d[i] = free_values.count_less(p[i]);
        free_values.erase(p[i]);
    }
    return d;
}

// O(n^2): берём d[i]-й по величине из оставшихся чисел (удаление из вектора).
inline Perm lehmer_to_perm_naive(const LehmerCode& d) {
    const auto n = static_cast<std::uint32_t>(d.size());
    std::vector<std::uint32_t> rest(n);
    std::iota(rest.begin(), rest.end(), 0u);
    Perm p(n);
    for (std::uint32_t i = 0; i < n; ++i) {
        if (d[i] >= rest.size()) throw std::invalid_argument("некорректный код Лемера");
        p[i] = rest[d[i]];
        rest.erase(rest.begin() + d[i]);
    }
    return p;
}

// O(n log n): то же самое через поиск k-го свободного в дереве Фенвика.
inline Perm lehmer_to_perm_fenwick(const LehmerCode& d) {
    const auto n = static_cast<std::uint32_t>(d.size());
    FenwickSet free_values(n);
    Perm p(n);
    for (std::uint32_t i = 0; i < n; ++i) {
        if (d[i] >= n - i) throw std::invalid_argument("некорректный код Лемера");
        p[i] = free_values.kth(d[i]);
        free_values.erase(p[i]);
    }
    return p;
}

// ---------- Шаг 2: код Лемера <-> номер ----------

// Схема Горнера в факториальной системе: r = (((d0·(n-1) + d1)·(n-2) + d2)·...)
template <class Num>
Num lehmer_to_rank(const LehmerCode& d) {
    const auto n = static_cast<std::uint32_t>(d.size());
    detail::require_perm_size<Num>(n);
    Num r = 0;
    for (std::uint32_t i = 0; i < n; ++i) {
        if (d[i] >= n - i) throw std::invalid_argument("некорректный код Лемера");
        num::mul_small(r, n - i);
        num::add_small(r, d[i]);
    }
    return r;
}

// Обратно: последовательно делим на 1, 2, ..., n и забираем остатки.
template <class Num>
LehmerCode rank_to_lehmer(Num r, std::uint32_t n) {
    detail::require_perm_size<Num>(n);
    LehmerCode d(n);
    for (std::uint32_t i = n; i-- > 0;) d[i] = num::divmod_small(r, n - i);
    if (!num::is_zero(r)) throw std::out_of_range("номер перестановки должен быть меньше n!");
    return d;
}

// ---------- Лексикографический порядок ----------

template <class Num>
Num perm_rank_lex_naive(const Perm& p) {
    detail::require_permutation(p);
    return lehmer_to_rank<Num>(lehmer_code_naive(p));
}

template <class Num>
Num perm_rank_lex(const Perm& p) {
    detail::require_permutation(p);
    return lehmer_to_rank<Num>(lehmer_code_fenwick(p));
}

template <class Num>
Perm perm_unrank_lex_naive(Num r, std::uint32_t n) {
    return lehmer_to_perm_naive(rank_to_lehmer<Num>(std::move(r), n));
}

template <class Num>
Perm perm_unrank_lex(Num r, std::uint32_t n) {
    return lehmer_to_perm_fenwick(rank_to_lehmer<Num>(std::move(r), n));
}

// ---------- Myrvold & Ruskey (2001): линейное время, свой порядок ----------
//
// select: начинаем с тождественной перестановки и для m = n, n-1, ..., 1
//         меняем местами p[m-1] и p[r mod m], затем r = r div m.
// rank:   «откатываем» эти обмены с конца, используя обратную перестановку,
//         и собираем цифры r mod m обратно в число.

template <class Num>
Num perm_rank_mr(const Perm& p_in) {
    detail::require_permutation(p_in);
    detail::require_perm_size<Num>(p_in.size());
    Perm p = p_in;
    const auto n = static_cast<std::uint32_t>(p.size());
    Perm inv(n);
    for (std::uint32_t i = 0; i < n; ++i) inv[p[i]] = i;

    std::vector<std::uint32_t> digit(n, 0);  // digit[m-1] = r mod m на шаге m
    for (std::uint32_t m = n; m > 1; --m) {
        const std::uint32_t s = p[m - 1];
        std::swap(p[m - 1], p[inv[m - 1]]);
        std::swap(inv[s], inv[m - 1]);
        digit[m - 1] = s;
    }
    Num r = 0;  // r = digit_n + n·(digit_{n-1} + (n-1)·(...))
    for (std::uint32_t m = 2; m <= n; ++m) {
        num::mul_small(r, m);
        num::add_small(r, digit[m - 1]);
    }
    return r;
}

template <class Num>
Perm perm_unrank_mr(Num r, std::uint32_t n) {
    detail::require_perm_size<Num>(n);
    Perm p(n);
    std::iota(p.begin(), p.end(), 0u);
    for (std::uint32_t m = n; m > 0; --m) {
        const std::uint32_t d = num::divmod_small(r, m);
        std::swap(p[m - 1], p[d]);
    }
    if (!num::is_zero(r)) throw std::out_of_range("номер перестановки должен быть меньше n!");
    return p;
}

}  // namespace combi
