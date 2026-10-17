// Мини-библиотека проверок без внешних зависимостей.
//   CHECK(условие)          — условие должно быть истинным;
//   CHECK_EQ(a, b)          — значения должны совпасть (при ошибке печатаются оба);
//   CHECK_THROWS(выражение) — выражение должно бросить исключение.
// Печатаются первые 20 провалов, дальше только считаются.
#pragma once

#include <cstdint>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace check {

struct Stats {
    long long passed = 0;
    long long failed = 0;
};

inline Stats& stats() {
    static Stats s;
    return s;
}

template <class T>
std::string show(const T& x) {
    std::ostringstream os;
    os << x;
    return os.str();
}

inline std::string show(const std::vector<std::uint32_t>& v) {
    std::string s = "{";
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i > 0) s += ", ";
        s += std::to_string(v[i]);
    }
    return s + "}";
}

inline void fail(const char* file, int line, const std::string& message) {
    auto& s = stats();
    ++s.failed;
    if (s.failed <= 20) std::cerr << file << ":" << line << ": ПРОВАЛ: " << message << "\n";
}

}  // namespace check

#define CHECK(cond)                                         \
    do {                                                    \
        if (cond) {                                         \
            ++check::stats().passed;                        \
        } else {                                            \
            check::fail(__FILE__, __LINE__, #cond);         \
        }                                                   \
    } while (0)

#define CHECK_EQ(a, b)                                                                     \
    do {                                                                                   \
        const auto& check_a_ = (a);                                                        \
        const auto& check_b_ = (b);                                                        \
        if (check_a_ == check_b_) {                                                        \
            ++check::stats().passed;                                                       \
        } else {                                                                           \
            check::fail(__FILE__, __LINE__,                                                \
                        std::string(#a " == " #b "\n    слева:  ") + check::show(check_a_) + \
                            "\n    справа: " + check::show(check_b_));                     \
        }                                                                                  \
    } while (0)

#define CHECK_THROWS(expr)                                                \
    do {                                                                  \
        bool check_thrown_ = false;                                       \
        try {                                                             \
            (void)(expr);                                                 \
        } catch (const std::exception&) {                                 \
            check_thrown_ = true;                                         \
        }                                                                 \
        if (check_thrown_) {                                              \
            ++check::stats().passed;                                      \
        } else {                                                          \
            check::fail(__FILE__, __LINE__, "ожидалось исключение: " #expr); \
        }                                                                 \
    } while (0)
