// Тесты сочетаний.
// Для маленьких n перебираем все сочетания: в лексикографическом порядке их
// выдаёт простой генератор, colex-порядок получаем сортировкой «с конца».
// Для больших n — select(rank(c)) == c, крайние номера и эталоны из Python.
#include <algorithm>
#include <cstdint>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "check.hpp"
#include "combi/combi.hpp"

using combi::BigUInt;
using combi::Comb;

namespace {

// Все k-сочетания из n в лексикографическом порядке.
std::vector<Comb> all_lex(std::uint32_t n, std::uint32_t k) {
    std::vector<Comb> out;
    Comb c(k);
    std::iota(c.begin(), c.end(), 0u);
    while (true) {
        out.push_back(c);
        // Ищем самую правую позицию, которую ещё можно увеличить.
        std::uint32_t i = k;
        while (i > 0 && c[i - 1] == n - k + (i - 1)) --i;
        if (i == 0) break;
        ++c[i - 1];
        for (std::uint32_t j = i; j < k; ++j) c[j] = c[j - 1] + 1;
    }
    return out;
}

bool colex_less(const Comb& a, const Comb& b) {
    return std::lexicographical_compare(a.rbegin(), a.rend(), b.rbegin(), b.rend());
}

Comb random_comb(std::uint32_t n, std::uint32_t k, std::mt19937_64& rng) {
    std::vector<std::uint32_t> all(n);
    std::iota(all.begin(), all.end(), 0u);
    for (std::uint32_t i = 0; i < k; ++i) {  // частичная перетасовка Фишера–Йетса
        const auto j = static_cast<std::uint32_t>(i + rng() % (n - i));
        std::swap(all[i], all[j]);
    }
    Comb c(all.begin(), all.begin() + k);
    std::sort(c.begin(), c.end());
    return c;
}

Comb first_comb(std::uint32_t k) {
    Comb c(k);
    std::iota(c.begin(), c.end(), 0u);
    return c;
}

Comb last_comb(std::uint32_t n, std::uint32_t k) {
    Comb c(k);
    std::iota(c.begin(), c.end(), n - k);
    return c;
}

}  // namespace

void test_combinations() {
    // --- Биномы ---
    CHECK_EQ(combi::binom_u64(67, 33), 14226520737620288370ull);
    CHECK_EQ(combi::binom_big(67, 33).to_u64(), combi::binom_u64(67, 33));
    CHECK_THROWS(combi::binom_u64(68, 34));
    CHECK_EQ(combi::binom_big(100, 50).to_string(), std::string("100891344545564193334812497256"));
    CHECK_EQ(combi::binom_big(1000, 500).to_string(),
             std::string("27028824094543656951561469362597527549615200844654828700739287510662542870552219389861248"
                         "39245023701653626060850215461048022097500506799175498942196995184754236654842637517333561"
                         "62464079737887344364574161119497604571044985756287880514600994219426752366915856603136862"
                         "602484428109296905863799821216320"));
    CHECK_EQ(combi::binom_big(5, 7), BigUInt(0));

    // --- Полный перебор: n ≤ 16, все k ---
    for (std::uint32_t n = 0; n <= 16; ++n) {
        for (std::uint32_t k = 0; k <= n; ++k) {
            std::vector<Comb> lex = all_lex(n, k);
            CHECK_EQ(static_cast<std::uint64_t>(lex.size()), combi::binom_u64(n, k));
            for (std::uint64_t idx = 0; idx < lex.size(); ++idx) {
                const Comb& c = lex[idx];
                CHECK_EQ(combi::comb_rank_lex_u64(c, n), idx);
                CHECK_EQ(combi::comb_unrank_lex_u64(idx, n, k), c);
                CHECK_EQ(combi::comb_rank_lex_big(c, n), BigUInt(idx));
                CHECK_EQ(combi::comb_unrank_lex_big(BigUInt(idx), n, k), c);
            }
            std::vector<Comb> colex = lex;
            std::sort(colex.begin(), colex.end(), colex_less);
            for (std::uint64_t idx = 0; idx < colex.size(); ++idx) {
                const Comb& c = colex[idx];
                CHECK_EQ(combi::comb_rank_colex_u64(c, n), idx);
                CHECK_EQ(combi::comb_unrank_colex_u64(idx, n, k), c);
                CHECK_EQ(combi::comb_rank_colex_big(c, n), BigUInt(idx));
                CHECK_EQ(combi::comb_unrank_colex_big(BigUInt(idx), n, k), c);
            }
        }
    }

    // --- Примеры из docs/theory.md ---
    CHECK_EQ(combi::comb_rank_colex_u64({0, 2, 4}, 5), 5ull);
    CHECK_EQ(combi::comb_rank_lex_u64({0, 2, 4}, 5), 4ull);

    // --- Ошибочный ввод ---
    CHECK_THROWS(combi::comb_rank_lex_u64({2, 1}, 5));     // не возрастает
    CHECK_THROWS(combi::comb_rank_lex_u64({1, 1}, 5));     // повтор
    CHECK_THROWS(combi::comb_rank_lex_big({1, 5}, 5));     // элемент ≥ n
    CHECK_THROWS(combi::comb_unrank_lex_u64(10, 5, 3));    // C(5,3) = 10 — вне диапазона
    CHECK_THROWS(combi::comb_unrank_colex_big(BigUInt(10), 5, 3));
    CHECK_THROWS(combi::comb_unrank_lex_big(BigUInt(0), 3, 4));  // k > n
    CHECK_THROWS(combi::comb_rank_lex_u64({0, 1}, 68));    // n > 67 не для u64

    // --- uint64 и BigUInt согласованы при n ≤ 67 ---
    std::mt19937_64 rng(2027);
    for (int it = 0; it < 3000; ++it) {
        const auto n = static_cast<std::uint32_t>(1 + rng() % 67);
        const auto k = static_cast<std::uint32_t>(rng() % (n + 1));
        const Comb c = random_comb(n, k, rng);
        const std::uint64_t rl = combi::comb_rank_lex_u64(c, n);
        const std::uint64_t rc = combi::comb_rank_colex_u64(c, n);
        CHECK_EQ(combi::comb_rank_lex_big(c, n).to_u64(), rl);
        CHECK_EQ(combi::comb_rank_colex_big(c, n).to_u64(), rc);
        CHECK_EQ(combi::comb_unrank_lex_u64(rl, n, k), c);
        CHECK_EQ(combi::comb_unrank_colex_u64(rc, n, k), c);
    }

    // --- Эталоны (tools/reference.py): n = 100, k = 30, c_i = 3i + 1 ---
    Comb c100(30);
    for (std::uint32_t i = 0; i < 30; ++i) c100[i] = 3 * i + 1;
    const BigUInt lex100 = BigUInt::from_string("12517731980142374647442978");
    const BigUInt colex100 = BigUInt::from_string("350255363780950760326978");
    CHECK_EQ(combi::comb_rank_lex_big(c100, 100), lex100);
    CHECK_EQ(combi::comb_rank_colex_big(c100, 100), colex100);
    CHECK_EQ(combi::comb_unrank_lex_big(lex100, 100, 30), c100);
    CHECK_EQ(combi::comb_unrank_colex_big(colex100, 100, 30), c100);

    // --- Большие n: крайние номера и «туда-обратно» ---
    const std::uint32_t sizes[][2] = {{1000, 500}, {5000, 37}, {10000, 9990}, {3000, 1}, {3000, 2999}};
    for (const auto& nk : sizes) {
        const std::uint32_t n = nk[0];
        const std::uint32_t k = nk[1];
        const BigUInt last = combi::binom_big(n, k) - BigUInt(1);
        CHECK_EQ(combi::comb_rank_lex_big(first_comb(k), n), BigUInt(0));
        CHECK_EQ(combi::comb_rank_lex_big(last_comb(n, k), n), last);
        CHECK_EQ(combi::comb_rank_colex_big(first_comb(k), n), BigUInt(0));
        CHECK_EQ(combi::comb_rank_colex_big(last_comb(n, k), n), last);
        CHECK_EQ(combi::comb_unrank_lex_big(BigUInt(0), n, k), first_comb(k));
        CHECK_EQ(combi::comb_unrank_lex_big(last, n, k), last_comb(n, k));
        for (int it = 0; it < 5; ++it) {
            const Comb c = random_comb(n, k, rng);
            CHECK_EQ(combi::comb_unrank_lex_big(combi::comb_rank_lex_big(c, n), n, k), c);
            CHECK_EQ(combi::comb_unrank_colex_big(combi::comb_rank_colex_big(c, n), n, k), c);
        }
    }
}
