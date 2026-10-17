// Тесты длинной арифметики: сверка с обычными 64-битными числами на случайных
// данных, свойства операций на больших числах и эталоны из tools/reference.py.
#include <cstdint>
#include <random>
#include <string>

#include "check.hpp"
#include "combi/combi.hpp"

using combi::BigUInt;

namespace {

std::string random_decimal(std::mt19937_64& rng, std::size_t len) {
    std::string s(len, '0');
    s[0] = static_cast<char>('1' + rng() % 9);
    for (std::size_t i = 1; i < len; ++i) s[i] = static_cast<char>('0' + rng() % 10);
    return s;
}

}  // namespace

void test_biguint() {
    // --- Строки ---
    CHECK_EQ(BigUInt().to_string(), std::string("0"));
    CHECK_EQ(BigUInt(18446744073709551615ull).to_string(), std::string("18446744073709551615"));
    const BigUInt two64 = BigUInt::from_string("18446744073709551616");
    CHECK_EQ(two64.bit_length(), std::size_t{65});
    CHECK(!two64.fits_u64());
    CHECK_EQ(BigUInt::from_string("000123").to_string(), std::string("123"));
    CHECK_THROWS(BigUInt::from_string(""));
    CHECK_THROWS(BigUInt::from_string("12a3"));
    CHECK_THROWS(two64.to_u64());

    // --- Сверка с uint64 на случайных числах ---
    std::mt19937_64 rng(2026);
    for (int it = 0; it < 20000; ++it) {
        const std::uint64_t a = rng() >> 1;  // < 2^63, чтобы сумма не переполнилась
        const std::uint64_t b = rng() >> 1;
        const auto m = static_cast<std::uint32_t>(rng() % 0xFFFFFFFFu) + 1;
        const std::uint64_t small = rng() >> 32;  // < 2^32, чтобы произведение влезло

        CHECK_EQ((BigUInt(a) + BigUInt(b)).to_u64(), a + b);
        CHECK_EQ((BigUInt(a > b ? a : b) - BigUInt(a > b ? b : a)).to_u64(), a > b ? a - b : b - a);
        CHECK_EQ(BigUInt(small).mul_small(m).to_u64(), small * m);
        BigUInt q(a);
        const std::uint32_t rem = q.divmod_small(m);
        CHECK_EQ(q.to_u64(), a / m);
        CHECK_EQ(static_cast<std::uint64_t>(rem), a % m);
        CHECK_EQ(BigUInt(a) < BigUInt(b), a < b);
        CHECK_EQ(BigUInt(a).to_string(), std::to_string(a));
    }

    // --- Свойства на больших числах (сотни цифр) ---
    for (int it = 0; it < 300; ++it) {
        const std::string sx = random_decimal(rng, 1 + rng() % 400);
        const std::string sy = random_decimal(rng, 1 + rng() % 400);
        const BigUInt x = BigUInt::from_string(sx);
        const BigUInt y = BigUInt::from_string(sy);
        CHECK_EQ(x.to_string(), sx);                 // строка -> число -> строка
        CHECK_EQ((x + y) - y, x);                    // сложение и вычитание обратны
        CHECK_EQ(x + y, y + x);
        const auto m = static_cast<std::uint32_t>(rng() % 0xFFFFFFFFu) + 1;
        const auto a = static_cast<std::uint32_t>(rng() % m);
        BigUInt z = x;
        z.mul_small(m).add_small(a);                 // z = x·m + a
        CHECK_EQ(z.divmod_small(m), a);              // z div m = x, z mod m = a
        CHECK_EQ(z, x);
        if (x < y) CHECK_THROWS(x - y);
    }

    // --- Эталоны (tools/reference.py) ---
    CHECK_EQ(combi::factorial<BigUInt>(100).to_string(),
             std::string("93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463"
                         "976156518286253697920827223758251185210916864000000000000000000000000"));
    CHECK_EQ(combi::factorial<std::uint64_t>(20), 2432902008176640000ull);
    CHECK_THROWS(combi::factorial<std::uint64_t>(21));
}
