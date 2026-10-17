// Тесты перестановок.
// Главная идея: для маленьких n перебираем ВСЕ перестановки в правильном порядке
// (std::next_permutation выдаёт ровно лексикографический порядок) и сверяем
// каждый номер. Для больших n проверяем select(rank(p)) == p и эталоны из Python.
#include <algorithm>
#include <cstdint>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "check.hpp"
#include "combi/combi.hpp"

using combi::BigUInt;
using combi::Perm;

namespace {

Perm identity(std::uint32_t n) {
    Perm p(n);
    std::iota(p.begin(), p.end(), 0u);
    return p;
}

Perm random_perm(std::uint32_t n, std::mt19937_64& rng) {
    Perm p = identity(n);
    std::shuffle(p.begin(), p.end(), rng);
    return p;
}

template <class Num>
void check_all_lex(std::uint32_t n) {
    Perm p = identity(n);
    std::uint64_t idx = 0;
    do {
        const Num expected = Num(idx);
        CHECK_EQ(combi::perm_rank_lex_naive<Num>(p), expected);
        CHECK_EQ(combi::perm_rank_lex<Num>(p), expected);
        CHECK_EQ(combi::perm_unrank_lex_naive<Num>(expected, n), p);
        CHECK_EQ(combi::perm_unrank_lex<Num>(expected, n), p);
        ++idx;
    } while (std::next_permutation(p.begin(), p.end()));
    CHECK_EQ(idx, combi::factorial<std::uint64_t>(n));
}

// Для Myrvold–Ruskey порядок другой, поэтому проверяем, что rank — биекция
// на {0, ..., n!-1}, а select — обратная к нему.
void check_all_mr(std::uint32_t n) {
    const std::uint64_t total = combi::factorial<std::uint64_t>(n);
    std::vector<bool> seen(total, false);
    Perm p = identity(n);
    do {
        const std::uint64_t r = combi::perm_rank_mr<std::uint64_t>(p);
        CHECK(r < total);
        if (r < total) {
            CHECK(!seen[r]);
            seen[r] = true;
        }
        CHECK_EQ(combi::perm_unrank_mr<std::uint64_t>(r, n), p);
        CHECK_EQ(combi::perm_rank_mr<BigUInt>(p), BigUInt(r));
    } while (std::next_permutation(p.begin(), p.end()));
    for (std::uint64_t r = 0; r < total; ++r) {
        CHECK_EQ(combi::perm_rank_mr<std::uint64_t>(combi::perm_unrank_mr<std::uint64_t>(r, n)), r);
    }
}

}  // namespace

void test_permutations() {
    // --- Полный перебор ---
    for (std::uint32_t n = 0; n <= 8; ++n) {
        check_all_lex<std::uint64_t>(n);
        check_all_lex<BigUInt>(n);
        check_all_mr(n);
    }

    // --- Пример из docs/theory.md ---
    CHECK_EQ(combi::perm_rank_lex<std::uint64_t>({2, 0, 3, 1}), 13ull);
    CHECK_EQ(combi::perm_unrank_lex<std::uint64_t>(13, 4), (Perm{2, 0, 3, 1}));
    CHECK_EQ(combi::lehmer_code_fenwick({2, 0, 3, 1}), (combi::LehmerCode{2, 0, 1, 0}));

    // --- Границы uint64: 20! помещается, 21! нет ---
    Perm rev20 = identity(20);
    std::reverse(rev20.begin(), rev20.end());
    CHECK_EQ(combi::perm_rank_lex<std::uint64_t>(rev20), 2432902008176639999ull);  // 20! - 1
    CHECK_EQ(combi::perm_unrank_lex<std::uint64_t>(2432902008176639999ull, 20), rev20);
    CHECK_THROWS(combi::perm_rank_lex<std::uint64_t>(identity(21)));
    CHECK_EQ(combi::perm_rank_lex<BigUInt>(identity(21)), BigUInt(0));

    // --- Ошибочный ввод ---
    CHECK_THROWS(combi::perm_rank_lex<std::uint64_t>({0, 0, 1}));
    CHECK_THROWS(combi::perm_rank_lex<std::uint64_t>({0, 3, 1}));
    CHECK_THROWS(combi::perm_rank_mr<std::uint64_t>({1, 1}));
    CHECK_THROWS(combi::perm_unrank_lex<std::uint64_t>(24, 4));  // 4! = 24 — уже вне диапазона
    CHECK_THROWS(combi::perm_unrank_lex_naive<BigUInt>(BigUInt(24), 4));
    CHECK_THROWS(combi::perm_unrank_mr<std::uint64_t>(24, 4));

    // --- Эталоны (tools/reference.py): n = 50, p_i = (7i + 3) mod 50 ---
    Perm p50(50);
    for (std::uint32_t i = 0; i < 50; ++i) p50[i] = (7 * i + 3) % 50;
    const BigUInt lex50 = BigUInt::from_string("1940569056005825838246977378223323801842238182638076466918898000");
    const BigUInt mr50 = BigUInt::from_string("26608544722806902259622853795788268479495321076081584588259305196");
    CHECK_EQ(combi::perm_rank_lex_naive<BigUInt>(p50), lex50);
    CHECK_EQ(combi::perm_rank_lex<BigUInt>(p50), lex50);
    CHECK_EQ(combi::perm_unrank_lex<BigUInt>(lex50, 50), p50);
    CHECK_EQ(combi::perm_rank_mr<BigUInt>(p50), mr50);
    CHECK_EQ(combi::perm_unrank_mr<BigUInt>(mr50, 50), p50);

    // --- Последняя перестановка n = 1000 имеет номер 1000! - 1 ---
    Perm rev1000 = identity(1000);
    std::reverse(rev1000.begin(), rev1000.end());
    const BigUInt last1000 = combi::factorial<BigUInt>(1000) - BigUInt(1);
    CHECK_EQ(combi::perm_rank_lex<BigUInt>(rev1000), last1000);
    CHECK_EQ(combi::perm_unrank_lex<BigUInt>(last1000, 1000), rev1000);

    // --- Случайные большие перестановки: туда и обратно, все методы согласованы ---
    std::mt19937_64 rng(60);
    for (std::uint32_t n : {100u, 1000u, 3000u}) {
        for (int it = 0; it < 3; ++it) {
            const Perm p = random_perm(n, rng);
            const BigUInt r = combi::perm_rank_lex<BigUInt>(p);
            CHECK_EQ(combi::perm_rank_lex_naive<BigUInt>(p), r);
            CHECK_EQ(combi::perm_unrank_lex<BigUInt>(r, n), p);
            CHECK_EQ(combi::perm_unrank_lex_naive<BigUInt>(r, n), p);
            CHECK_EQ(combi::perm_unrank_mr<BigUInt>(combi::perm_rank_mr<BigUInt>(p), n), p);
        }
    }

    // --- uint64 и BigUInt дают одинаковые номера при n ≤ 20 ---
    for (int it = 0; it < 2000; ++it) {
        const auto n = static_cast<std::uint32_t>(rng() % 21);
        const Perm p = random_perm(n, rng);
        CHECK_EQ(combi::perm_rank_lex<BigUInt>(p).to_u64(), combi::perm_rank_lex<std::uint64_t>(p));
        CHECK_EQ(combi::perm_rank_mr<BigUInt>(p).to_u64(), combi::perm_rank_mr<std::uint64_t>(p));
    }
}
