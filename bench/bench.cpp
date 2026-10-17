// Замеры скорости rank/select.
//
// Запуск:  bench          — полный набор (до минуты)
//          bench --quick  — короткий прогон (CI, быстрая проверка)
// Результат: таблица в консоли и файл bench_results.csv (для графиков в отчёте).
//
// Число повторов подбирается само: каждая строка меряется ~0.3 с (в --quick ~0.03 с).
// Собирайте в режиме Release, иначе цифры бессмысленны.
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "combi/combi.hpp"
#include "combi/console.hpp"

using combi::BigUInt;
using combi::Comb;
using combi::Perm;

namespace {

volatile std::uint32_t g_sink = 0;  // чтобы компилятор не выбросил «ненужные» вычисления
double g_target_ns = 3e8;

struct Row {
    std::string object;
    std::string algorithm;
    std::string number_type;
    std::uint32_t n;
    std::uint32_t k;
    double ns_per_op;
    long reps;
};
std::vector<Row> g_rows;

// Дополнить строку пробелами до width символов (UTF-8: кириллица — 2 байта на символ).
std::string pad(const std::string& s, std::size_t width) {
    std::size_t chars = 0;
    for (char c : s) {
        if ((static_cast<unsigned char>(c) & 0xC0u) != 0x80u) ++chars;
    }
    return chars >= width ? s + " " : s + std::string(width - chars, ' ');
}

std::string human(double ns) {
    char buf[64];
    if (ns < 1e3) {
        std::snprintf(buf, sizeof buf, "%.0f нс", ns);
    } else if (ns < 1e6) {
        std::snprintf(buf, sizeof buf, "%.2f мкс", ns / 1e3);
    } else if (ns < 1e9) {
        std::snprintf(buf, sizeof buf, "%.2f мс", ns / 1e6);
    } else {
        std::snprintf(buf, sizeof buf, "%.2f с", ns / 1e9);
    }
    return buf;
}

template <class F>
void measure(const std::string& object, const std::string& algorithm, const std::string& number_type,
             std::uint32_t n, std::uint32_t k, F&& f) {
    using clock = std::chrono::steady_clock;
    auto t0 = clock::now();
    f();  // прогрев + оценка времени одного вызова
    const double once = std::chrono::duration<double, std::nano>(clock::now() - t0).count();
    const long reps = std::max(1L, std::min(2000000L, static_cast<long>(g_target_ns / std::max(once, 1.0))));
    t0 = clock::now();
    for (long i = 0; i < reps; ++i) f();
    const double ns = std::chrono::duration<double, std::nano>(clock::now() - t0).count() / static_cast<double>(reps);
    g_rows.push_back({object, algorithm, number_type, n, k, ns, reps});

    std::string size = "n=" + std::to_string(n);
    if (k != 0) size += " k=" + std::to_string(k);
    std::cout << "  " << pad(algorithm, 36) << pad(number_type, 9) << pad(size, 16) << human(ns) << "\n";
}

template <class Num>
void bench_permutations(std::uint32_t n, bool with_naive, std::mt19937_64& rng) {
    const char* nt = combi::NumTraits<Num>::name;
    Perm p(n);
    std::iota(p.begin(), p.end(), 0u);
    std::shuffle(p.begin(), p.end(), rng);
    const Num r = combi::perm_rank_lex<Num>(p);
    const Num r_mr = combi::perm_rank_mr<Num>(p);
    const combi::LehmerCode d = combi::lehmer_code_fenwick(p);

    std::cout << "Перестановки, n = " << n << " (" << nt << ")\n";
    if (with_naive) {
        measure("perm", "rank lex наивно O(n^2)", nt, n, 0,
                [&] { g_sink = g_sink + combi::num::low_bits(combi::perm_rank_lex_naive<Num>(p)); });
    }
    measure("perm", "rank lex Фенвик O(n log n)", nt, n, 0,
            [&] { g_sink = g_sink + combi::num::low_bits(combi::perm_rank_lex<Num>(p)); });
    measure("perm", "  из них: код Лемера (Фенвик)", nt, n, 0,
            [&] { g_sink = g_sink + combi::lehmer_code_fenwick(p)[0]; });
    measure("perm", "  из них: Лемер -> номер (Горнер)", nt, n, 0,
            [&] { g_sink = g_sink + combi::num::low_bits(combi::lehmer_to_rank<Num>(d)); });
    measure("perm", "rank Myrvold-Ruskey O(n)", nt, n, 0,
            [&] { g_sink = g_sink + combi::num::low_bits(combi::perm_rank_mr<Num>(p)); });
    if (with_naive) {
        measure("perm", "select lex наивно O(n^2)", nt, n, 0,
                [&] { g_sink = g_sink + combi::perm_unrank_lex_naive<Num>(r, n)[0]; });
    }
    measure("perm", "select lex Фенвик O(n log n)", nt, n, 0,
            [&] { g_sink = g_sink + combi::perm_unrank_lex<Num>(r, n)[0]; });
    measure("perm", "select Myrvold-Ruskey O(n)", nt, n, 0,
            [&] { g_sink = g_sink + combi::perm_unrank_mr<Num>(r_mr, n)[0]; });
}

Comb random_comb(std::uint32_t n, std::uint32_t k, std::mt19937_64& rng) {
    std::vector<std::uint32_t> all(n);
    std::iota(all.begin(), all.end(), 0u);
    std::shuffle(all.begin(), all.end(), rng);
    Comb c(all.begin(), all.begin() + k);
    std::sort(c.begin(), c.end());
    return c;
}

void bench_combinations_u64(std::uint32_t n, std::uint32_t k, std::mt19937_64& rng) {
    const Comb c = random_comb(n, k, rng);
    const std::uint64_t rl = combi::comb_rank_lex_u64(c, n);
    const std::uint64_t rc = combi::comb_rank_colex_u64(c, n);
    std::cout << "Сочетания, n = " << n << ", k = " << k << " (uint64, таблица биномов)\n";
    measure("comb", "rank lex", "uint64", n, k, [&] { g_sink = g_sink + combi::num::low_bits(combi::comb_rank_lex_u64(c, n)); });
    measure("comb", "rank colex", "uint64", n, k, [&] { g_sink = g_sink + combi::num::low_bits(combi::comb_rank_colex_u64(c, n)); });
    measure("comb", "select lex", "uint64", n, k, [&] { g_sink = g_sink + combi::comb_unrank_lex_u64(rl, n, k)[0]; });
    measure("comb", "select colex", "uint64", n, k, [&] { g_sink = g_sink + combi::comb_unrank_colex_u64(rc, n, k)[0]; });
}

void bench_combinations_big(std::uint32_t n, std::uint32_t k, std::mt19937_64& rng) {
    const Comb c = random_comb(n, k, rng);
    const BigUInt rl = combi::comb_rank_lex_big(c, n);
    const BigUInt rc = combi::comb_rank_colex_big(c, n);
    std::cout << "Сочетания, n = " << n << ", k = " << k << " (BigUInt)\n";
    measure("comb", "rank lex", "BigUInt", n, k, [&] { g_sink = g_sink + combi::comb_rank_lex_big(c, n).low_limb(); });
    measure("comb", "rank colex", "BigUInt", n, k, [&] { g_sink = g_sink + combi::comb_rank_colex_big(c, n).low_limb(); });
    measure("comb", "select lex", "BigUInt", n, k, [&] { g_sink = g_sink + combi::comb_unrank_lex_big(rl, n, k)[0]; });
    measure("comb", "select colex", "BigUInt", n, k, [&] { g_sink = g_sink + combi::comb_unrank_colex_big(rc, n, k)[0]; });
}

void write_csv(const char* path) {
    std::ofstream out(path);
    out << "object,algorithm,number_type,n,k,ns_per_op,reps\n";
    for (const Row& r : g_rows) {
        out << r.object << ",\"" << r.algorithm << "\"," << r.number_type << "," << r.n << "," << r.k << ","
            << r.ns_per_op << "," << r.reps << "\n";
    }
}

}  // namespace

int main(int argc, char** argv) {
    combi::setup_console_utf8();
    const bool quick = argc > 1 && std::strcmp(argv[1], "--quick") == 0;
    if (quick) g_target_ns = 3e7;
    std::mt19937_64 rng(42);

    std::cout << "Замеры rank/select" << (quick ? " (быстрый режим)" : "") << "\n\n";

    bench_permutations<std::uint64_t>(20, true, rng);
    const std::vector<std::uint32_t> perm_sizes =
        quick ? std::vector<std::uint32_t>{20, 100, 1000} : std::vector<std::uint32_t>{20, 100, 1000, 10000, 30000};
    for (std::uint32_t n : perm_sizes) bench_permutations<BigUInt>(n, n <= 10000, rng);

    bench_combinations_u64(60, 30, rng);
    using NK = std::pair<std::uint32_t, std::uint32_t>;
    const std::vector<NK> comb_sizes =
        quick ? std::vector<NK>{{60, 30}, {1000, 500}} : std::vector<NK>{{60, 30}, {1000, 500}, {10000, 5000}, {100000, 50000}};
    for (const NK& nk : comb_sizes) bench_combinations_big(nk.first, nk.second, rng);

    write_csv("bench_results.csv");
    std::cout << "\nТаблица сохранена в bench_results.csv\n";
    return 0;
}
