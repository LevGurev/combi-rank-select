// rank/select сочетаний.
//
// Сочетание из n по k — возрастающий вектор c из k разных чисел 0..n-1.
// Два порядка:
//   лексикографический (lex):   {0,1,2} < {0,1,3} < {0,1,4} < {0,2,3} < ...
//   колексикографический (colex): сравниваем с конца, по наибольшему элементу:
//                                 {0,1,2} < {0,1,3} < {0,2,3} < {1,2,3} < {0,1,4} < ...
//
// Colex-номер — это «комбинаторная система счисления»:
//   rank_colex(c) = C(c_0, 1) + C(c_1, 2) + ... + C(c_{k-1}, k).
// Lex сводится к colex отражением x -> n-1-x:
//   rank_lex(c) = C(n, k) - 1 - rank_colex(отражённое c).
// Выкладки с примерами — в docs/theory.md.
//
// Версии *_u64: n ≤ 67 (все C(n, k) помещаются в 64 бита), биномы из таблицы
//               Паскаля, O(n) операций.
// Версии *_big: любое n. Биномы не хранятся, а пересчитываются соседними шагами
//               (умножение и деление на короткое число), O(n) операций
//               с длинными числами.
#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#include "biguint.hpp"

namespace combi {

using Comb = std::vector<std::uint32_t>;

// Строго возрастает и все элементы меньше n.
inline bool is_combination(const Comb& c, std::uint32_t n) {
    for (std::size_t i = 0; i < c.size(); ++i) {
        if (c[i] >= n) return false;
        if (i > 0 && c[i - 1] >= c[i]) return false;
    }
    return true;
}

namespace detail {

constexpr std::uint32_t kMaxBinomN64 = 67;  // C(67, 33) < 2^64 < C(68, 34)

struct BinomTable64 {
    std::uint64_t c[kMaxBinomN64 + 1][kMaxBinomN64 + 1] = {};
    BinomTable64() {
        for (std::uint32_t n = 0; n <= kMaxBinomN64; ++n) {
            c[n][0] = 1;
            for (std::uint32_t k = 1; k <= n; ++k) c[n][k] = c[n - 1][k - 1] + c[n - 1][k];
        }
    }
};

inline const BinomTable64& binom_table64() {
    static const BinomTable64 table;
    return table;
}

inline void require_combination(const Comb& c, std::uint32_t n) {
    if (!is_combination(c, n)) {
        throw std::invalid_argument("ожидалось сочетание: возрастающие числа из 0..n-1");
    }
}

inline void require_n64(std::uint32_t n) {
    if (n > kMaxBinomN64) throw std::overflow_error("для n > 67 номера не помещаются в 64 бита; используйте *_big");
}

// Отражение x -> n-1-x с разворотом: превращает lex-задачу в colex-задачу.
inline Comb reflect(const Comb& c, std::uint32_t n) {
    const std::size_t k = c.size();
    Comb d(k);
    for (std::size_t i = 0; i < k; ++i) d[i] = n - 1 - c[k - 1 - i];
    return d;
}

}  // namespace detail

// ---------- Биномиальные коэффициенты ----------

inline std::uint64_t binom_u64(std::uint32_t n, std::uint32_t k) {
    detail::require_n64(n);
    if (k > n) return 0;
    return detail::binom_table64().c[n][k];
}

// C(n, k) = (n-k+1)/1 · (n-k+2)/2 · ... — каждое промежуточное значение целое.
inline BigUInt binom_big(std::uint32_t n, std::uint32_t k) {
    if (k > n) return BigUInt();
    k = std::min(k, n - k);
    BigUInt r = 1;
    for (std::uint32_t i = 1; i <= k; ++i) {
        r.mul_small(n - k + i);
        r.divmod_small(i);
    }
    return r;
}

// ---------- 64-битные версии (n ≤ 67) ----------

inline std::uint64_t comb_rank_colex_u64(const Comb& c, std::uint32_t n) {
    detail::require_n64(n);
    detail::require_combination(c, n);
    std::uint64_t r = 0;
    for (std::uint32_t i = 0; i < c.size(); ++i) r += binom_u64(c[i], i + 1);
    return r;
}

// Жадно, с наибольшего элемента: c_{i-1} — наибольшее x, для которого C(x, i) ≤ r.
inline Comb comb_unrank_colex_u64(std::uint64_t r, std::uint32_t n, std::uint32_t k) {
    detail::require_n64(n);
    if (k > n) throw std::invalid_argument("k > n");
    if (r >= binom_u64(n, k)) throw std::out_of_range("номер сочетания должен быть меньше C(n, k)");
    Comb c(k);
    std::uint32_t x = n;
    for (std::uint32_t i = k; i >= 1; --i) {
        do {
            --x;
        } while (binom_u64(x, i) > r);
        c[i - 1] = x;
        r -= binom_u64(x, i);
    }
    return c;
}

inline std::uint64_t comb_rank_lex_u64(const Comb& c, std::uint32_t n) {
    detail::require_n64(n);
    detail::require_combination(c, n);
    const auto k = static_cast<std::uint32_t>(c.size());
    return binom_u64(n, k) - 1 - comb_rank_colex_u64(detail::reflect(c, n), n);
}

inline Comb comb_unrank_lex_u64(std::uint64_t r, std::uint32_t n, std::uint32_t k) {
    detail::require_n64(n);
    if (k > n) throw std::invalid_argument("k > n");
    const std::uint64_t total = binom_u64(n, k);
    if (r >= total) throw std::out_of_range("номер сочетания должен быть меньше C(n, k)");
    return detail::reflect(comb_unrank_colex_u64(total - 1 - r, n, k), n);
}

// ---------- Версии с длинной арифметикой (любое n) ----------
//
// Соседние биномы пересчитываются точным умножением и делением:
//   C(m-1, j)   = C(m, j) · (m - j) / m
//   C(m-1, j-1) = C(m, j) · j / m
// Пока m ≥ j, бином не меньше 1, поэтому деление не «теряет» информацию.

inline BigUInt comb_rank_colex_big(const Comb& c, std::uint32_t n) {
    detail::require_combination(c, n);
    const auto k = static_cast<std::uint32_t>(c.size());
    BigUInt r;
    if (k == 0 || c[k - 1] == k - 1) return r;  // {0,1,...,k-1} — первое сочетание

    // Идём по слагаемым C(c_i, i+1) от последнего к первому; b == C(m, j), j == i+1.
    std::uint32_t m = c[k - 1];
    std::uint32_t j = k;
    BigUInt b = binom_big(m, j);
    for (std::uint32_t i = k; i-- > 0;) {
        if (c[i] == i) break;  // дальше c = {0,...,i}: оставшиеся слагаемые равны нулю
        while (m > c[i]) {
            b.mul_small(m - j);
            b.divmod_small(m);
            --m;
        }
        r += b;
        if (i == 0) break;
        b.mul_small(j);
        b.divmod_small(m);
        --m;
        --j;
    }
    return r;
}

namespace detail {

// Colex-select при уже посчитанном total = C(n, k).
inline Comb colex_unrank_big(BigUInt r, std::uint32_t n, std::uint32_t k, const BigUInt& total) {
    Comb c(k);
    if (k == 0) return c;
    // b == C(m, i); начинаем с m = n-1: C(n-1, k) = C(n, k) · (n-k) / n.
    std::uint32_t m = n - 1;
    BigUInt b = total;
    b.mul_small(n - k);
    b.divmod_small(n);
    for (std::uint32_t i = k; i >= 1; --i) {
        if (r.is_zero()) {  // остаток ноль: дальше идут самые маленькие числа 0, 1, ..., i-1
            for (std::uint32_t t = 0; t < i; ++t) c[t] = t;
            break;
        }
        while (b > r) {  // ищем наибольшее m с C(m, i) ≤ r
            b.mul_small(m - i);
            b.divmod_small(m);
            --m;
        }
        c[i - 1] = m;
        r -= b;
        if (i == 1) break;
        b.mul_small(i);  // переходим к C(m-1, i-1)
        b.divmod_small(m);
        --m;
    }
    return c;
}

}  // namespace detail

inline Comb comb_unrank_colex_big(BigUInt r, std::uint32_t n, std::uint32_t k) {
    if (k > n) throw std::invalid_argument("k > n");
    const BigUInt total = binom_big(n, k);
    if (r >= total) throw std::out_of_range("номер сочетания должен быть меньше C(n, k)");
    return detail::colex_unrank_big(std::move(r), n, k, total);
}

inline BigUInt comb_rank_lex_big(const Comb& c, std::uint32_t n) {
    detail::require_combination(c, n);
    const auto k = static_cast<std::uint32_t>(c.size());
    BigUInt r = binom_big(n, k);
    r -= BigUInt(1);
    r -= comb_rank_colex_big(detail::reflect(c, n), n);
    return r;
}

inline Comb comb_unrank_lex_big(const BigUInt& r, std::uint32_t n, std::uint32_t k) {
    if (k > n) throw std::invalid_argument("k > n");
    const BigUInt total = binom_big(n, k);
    if (r >= total) throw std::out_of_range("номер сочетания должен быть меньше C(n, k)");
    BigUInt rr = total;
    rr -= BigUInt(1);
    rr -= r;
    return detail::reflect(detail::colex_unrank_big(std::move(rr), n, k, total), n);
}

}  // namespace combi
