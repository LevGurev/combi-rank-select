// Демонстрация и мини-калькулятор rank/select.
//
//   demo                       — показать примеры
//   demo perm-rank 2 0 3 1     — номер перестановки (lex и Myrvold-Ruskey)
//   demo perm-select 4 13      — перестановка длины 4 с lex-номером 13
//   demo comb-rank 5 0 2 4     — номер сочетания {0,2,4} из 5 (lex и colex)
//   demo comb-select 5 3 4     — сочетание из 5 по 3 с номером 4 (lex и colex)
//
// Номера могут быть сколь угодно большими: везде используется BigUInt.
#include <algorithm>
#include <cstdint>
#include <exception>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "combi/combi.hpp"
#include "combi/console.hpp"

using combi::BigUInt;

namespace {

std::string show(const std::vector<std::uint32_t>& v) {
    std::string s;
    for (std::size_t i = 0; i < v.size(); ++i) s += (i ? " " : "") + std::to_string(v[i]);
    return s;
}

std::uint32_t to_u32(const std::string& s) { return static_cast<std::uint32_t>(std::stoul(s)); }

std::string short_number(const BigUInt& x) {
    const std::string s = x.to_string();
    if (s.size() <= 60) return s;
    return s.substr(0, 25) + "..." + s.substr(s.size() - 25) + "  (" + std::to_string(s.size()) + " цифр)";
}

void showcase() {
    std::cout << "1) Все перестановки 0 1 2 в лексикографическом порядке:\n";
    std::vector<std::uint32_t> p = {0, 1, 2};
    do {
        std::cout << "   rank = " << combi::perm_rank_lex<std::uint64_t>(p) << ":  " << show(p) << "\n";
    } while (std::next_permutation(p.begin(), p.end()));

    const combi::Perm q = {2, 0, 3, 1};
    std::cout << "\n2) rank(" << show(q) << "):\n"
              << "   код Лемера = (" << show(combi::lehmer_code_fenwick(q)) << ")\n"
              << "   номер = 2·3! + 0·2! + 1·1! + 0·0! = " << combi::perm_rank_lex<std::uint64_t>(q) << "\n"
              << "   select(13, n=4) = " << show(combi::perm_unrank_lex<std::uint64_t>(13, 4)) << "\n";

    std::vector<std::uint32_t> rev(30);
    std::iota(rev.rbegin(), rev.rend(), 0u);
    std::cout << "\n3) Последняя перестановка длины 30 (29 28 ... 0):\n"
              << "   rank = " << combi::perm_rank_lex<BigUInt>(rev) << " = 30! - 1\n"
              << "   (в 64 бита уже не влезает: там максимум n = 20)\n";

    std::cout << "\n4) Сочетания из 5 по 3 в colex-порядке (сравнение с конца):\n";
    for (std::uint64_t r = 0; r < combi::binom_u64(5, 3); ++r) {
        const combi::Comb c = combi::comb_unrank_colex_u64(r, 5, 3);
        std::cout << "   rank = " << r << ":  {" << show(c) << "}"
                  << "   lex-номер = " << combi::comb_rank_lex_u64(c, 5) << "\n";
    }

    combi::Comb big(500);
    for (std::uint32_t i = 0; i < 500; ++i) big[i] = 2 * i;  // {0, 2, 4, ..., 998}
    const BigUInt r = combi::comb_rank_lex_big(big, 1000);
    std::cout << "\n5) Сочетание {0, 2, 4, ..., 998} из C(1000, 500):\n"
              << "   всего сочетаний = " << short_number(combi::binom_big(1000, 500)) << "\n"
              << "   его lex-номер   = " << short_number(r) << "\n"
              << "   select(номер) совпал с исходным: "
              << (combi::comb_unrank_lex_big(r, 1000, 500) == big ? "да" : "НЕТ") << "\n";

    std::cout << "\nКалькулятор: demo perm-rank 2 0 3 1 | demo perm-select 4 13 | "
                 "demo comb-rank 5 0 2 4 | demo comb-select 5 3 4\n";
}

int run(const std::vector<std::string>& a) {
    const std::string& cmd = a[0];
    if (cmd == "perm-rank" && a.size() >= 2) {
        combi::Perm p;
        for (std::size_t i = 1; i < a.size(); ++i) p.push_back(to_u32(a[i]));
        const BigUInt lex = combi::perm_rank_lex<BigUInt>(p);
        const BigUInt mr = combi::perm_rank_mr<BigUInt>(p);
        std::cout << "lex rank: " << lex << "\nMyrvold-Ruskey rank: " << mr << "\n";
        return 0;
    }
    if (cmd == "perm-select" && a.size() == 3) {
        const std::uint32_t n = to_u32(a[1]);
        const BigUInt r = BigUInt::from_string(a[2]);
        const combi::Perm lex = combi::perm_unrank_lex<BigUInt>(r, n);
        const combi::Perm mr = combi::perm_unrank_mr<BigUInt>(r, n);
        std::cout << "lex: " << show(lex) << "\nMyrvold-Ruskey: " << show(mr) << "\n";
        return 0;
    }
    if (cmd == "comb-rank" && a.size() >= 2) {
        const std::uint32_t n = to_u32(a[1]);
        combi::Comb c;
        for (std::size_t i = 2; i < a.size(); ++i) c.push_back(to_u32(a[i]));
        const BigUInt lex = combi::comb_rank_lex_big(c, n);
        const BigUInt colex = combi::comb_rank_colex_big(c, n);
        std::cout << "lex rank: " << lex << "\ncolex rank: " << colex << "\n";
        return 0;
    }
    if (cmd == "comb-select" && a.size() == 4) {
        const std::uint32_t n = to_u32(a[1]);
        const std::uint32_t k = to_u32(a[2]);
        const BigUInt r = BigUInt::from_string(a[3]);
        const combi::Comb lex = combi::comb_unrank_lex_big(r, n, k);
        const combi::Comb colex = combi::comb_unrank_colex_big(r, n, k);
        std::cout << "lex: {" << show(lex) << "}\ncolex: {" << show(colex) << "}\n";
        return 0;
    }
    std::cerr << "Не понял команду. Примеры:\n"
                 "  demo perm-rank 2 0 3 1\n  demo perm-select 4 13\n"
                 "  demo comb-rank 5 0 2 4\n  demo comb-select 5 3 4\n";
    return 2;
}

}  // namespace

int main(int argc, char** argv) {
    combi::setup_console_utf8();
    try {
        if (argc < 2) {
            showcase();
            return 0;
        }
        return run(std::vector<std::string>(argv + 1, argv + argc));
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }
}
