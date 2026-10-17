// Минимальная длинная арифметика для rank/select.
//
// Почему своя, а не GMP/Boost: проект должен собираться у всех одной командой,
// без установки библиотек (в том числе на Windows с Visual Studio). Для
// rank/select перестановок и сочетаний хватает операций
//   «длинное ± длинное»  и  «длинное × или ÷ короткое (до 2^32)».
// Ускорить длинную арифметику — отдельная задача проекта (см. docs/theory.md).
//
// Число хранится в системе счисления с основанием 2^32: вектор «цифр» (limbs),
// младшая цифра первой, ведущих нулей нет. Ноль — пустой вектор.
#pragma once

#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace combi {

class BigUInt {
public:
    BigUInt() = default;

    // Неявное преобразование намеренно: удобно писать BigUInt x = 5; или x == 0.
    BigUInt(std::uint64_t v) {  // NOLINT(google-explicit-constructor)
        while (v != 0) {
            limbs_.push_back(static_cast<std::uint32_t>(v));
            v >>= 32;
        }
    }

    // ----- Преобразования в строку и обратно (десятичная запись) -----

    static BigUInt from_string(const std::string& s) {
        if (s.empty()) throw std::invalid_argument("BigUInt::from_string: пустая строка");
        for (char ch : s) {
            if (ch < '0' || ch > '9') throw std::invalid_argument("BigUInt::from_string: не цифра");
        }
        // Читаем блоками по 9 цифр: 10^9 < 2^32, поэтому блок — «короткое» число.
        BigUInt x;
        std::size_t pos = 0;
        std::size_t first = s.size() % 9;
        if (first == 0) first = 9;
        while (pos < s.size()) {
            const std::size_t len = (pos == 0) ? first : 9;
            std::uint32_t block = 0;
            std::uint32_t scale = 1;
            for (std::size_t i = 0; i < len; ++i) {
                block = block * 10 + static_cast<std::uint32_t>(s[pos + i] - '0');
                scale *= 10;
            }
            x.mul_small(scale);
            x.add_small(block);
            pos += len;
        }
        return x;
    }

    std::string to_string() const {
        if (is_zero()) return "0";
        BigUInt t = *this;
        std::vector<std::uint32_t> blocks;  // блоки по 9 десятичных цифр, младшие первыми
        while (!t.is_zero()) blocks.push_back(t.divmod_small(1000000000u));
        std::string out = std::to_string(blocks.back());
        for (std::size_t i = blocks.size() - 1; i-- > 0;) {
            std::string part = std::to_string(blocks[i]);
            out.append(9 - part.size(), '0');
            out += part;
        }
        return out;
    }

    // ----- Свойства -----

    bool is_zero() const { return limbs_.empty(); }
    std::size_t limb_count() const { return limbs_.size(); }
    std::uint32_t low_limb() const { return limbs_.empty() ? 0u : limbs_[0]; }

    // Сколько бит нужно, чтобы записать число (для нуля — 0).
    std::size_t bit_length() const {
        if (limbs_.empty()) return 0;
        std::uint32_t top = limbs_.back();
        std::size_t bits = 0;
        while (top != 0) {
            ++bits;
            top >>= 1;
        }
        return 32 * (limbs_.size() - 1) + bits;
    }

    bool fits_u64() const { return limbs_.size() <= 2; }

    std::uint64_t to_u64() const {
        if (!fits_u64()) throw std::overflow_error("BigUInt::to_u64: число не помещается в 64 бита");
        std::uint64_t v = 0;
        if (limbs_.size() > 1) v = static_cast<std::uint64_t>(limbs_[1]) << 32;
        if (!limbs_.empty()) v |= limbs_[0];
        return v;
    }

    // ----- Операции с коротким числом: O(длины числа) -----

    BigUInt& add_small(std::uint32_t a) {
        std::uint64_t carry = a;
        for (std::size_t i = 0; carry != 0 && i < limbs_.size(); ++i) {
            const std::uint64_t t = static_cast<std::uint64_t>(limbs_[i]) + carry;
            limbs_[i] = static_cast<std::uint32_t>(t);
            carry = t >> 32;
        }
        if (carry != 0) limbs_.push_back(static_cast<std::uint32_t>(carry));
        return *this;
    }

    BigUInt& mul_small(std::uint32_t m) {
        if (m == 0 || limbs_.empty()) {
            limbs_.clear();
            return *this;
        }
        std::uint64_t carry = 0;
        for (auto& limb : limbs_) {
            const std::uint64_t t = static_cast<std::uint64_t>(limb) * m + carry;
            limb = static_cast<std::uint32_t>(t);
            carry = t >> 32;
        }
        if (carry != 0) limbs_.push_back(static_cast<std::uint32_t>(carry));
        return *this;
    }

    // Делит число на m (на месте) и возвращает остаток.
    std::uint32_t divmod_small(std::uint32_t m) {
        if (m == 0) throw std::domain_error("BigUInt::divmod_small: деление на ноль");
        std::uint64_t rem = 0;
        for (std::size_t i = limbs_.size(); i-- > 0;) {
            const std::uint64_t cur = (rem << 32) | limbs_[i];
            limbs_[i] = static_cast<std::uint32_t>(cur / m);
            rem = cur % m;
        }
        trim();
        return static_cast<std::uint32_t>(rem);
    }

    // ----- Операции с длинным числом: O(длины числа) -----

    BigUInt& operator+=(const BigUInt& o) {
        if (limbs_.size() < o.limbs_.size()) limbs_.resize(o.limbs_.size(), 0);
        std::uint64_t carry = 0;
        for (std::size_t i = 0; i < limbs_.size(); ++i) {
            if (i >= o.limbs_.size() && carry == 0) break;
            const std::uint64_t t = static_cast<std::uint64_t>(limbs_[i]) + carry +
                                    (i < o.limbs_.size() ? o.limbs_[i] : 0u);
            limbs_[i] = static_cast<std::uint32_t>(t);
            carry = t >> 32;
        }
        if (carry != 0) limbs_.push_back(static_cast<std::uint32_t>(carry));
        return *this;
    }

    // Требует *this >= o (беззнаковые числа), иначе бросает исключение.
    BigUInt& operator-=(const BigUInt& o) {
        if (compare(*this, o) < 0) throw std::underflow_error("BigUInt: вычитание дало бы отрицательное число");
        std::uint64_t borrow = 0;
        for (std::size_t i = 0; i < limbs_.size(); ++i) {
            if (i >= o.limbs_.size() && borrow == 0) break;
            const std::uint64_t sub = (i < o.limbs_.size() ? o.limbs_[i] : 0u) + borrow;
            const std::uint64_t cur = limbs_[i];
            if (cur >= sub) {
                limbs_[i] = static_cast<std::uint32_t>(cur - sub);
                borrow = 0;
            } else {
                limbs_[i] = static_cast<std::uint32_t>(cur + (std::uint64_t{1} << 32) - sub);
                borrow = 1;
            }
        }
        trim();
        return *this;
    }

    friend BigUInt operator+(BigUInt a, const BigUInt& b) { return a += b; }
    friend BigUInt operator-(BigUInt a, const BigUInt& b) { return a -= b; }

    // ----- Сравнение -----

    // -1, 0, 1 — как strcmp.
    friend int compare(const BigUInt& a, const BigUInt& b) {
        if (a.limbs_.size() != b.limbs_.size()) return a.limbs_.size() < b.limbs_.size() ? -1 : 1;
        for (std::size_t i = a.limbs_.size(); i-- > 0;) {
            if (a.limbs_[i] != b.limbs_[i]) return a.limbs_[i] < b.limbs_[i] ? -1 : 1;
        }
        return 0;
    }

    friend bool operator==(const BigUInt& a, const BigUInt& b) { return a.limbs_ == b.limbs_; }
    friend bool operator!=(const BigUInt& a, const BigUInt& b) { return !(a == b); }
    friend bool operator<(const BigUInt& a, const BigUInt& b) { return compare(a, b) < 0; }
    friend bool operator<=(const BigUInt& a, const BigUInt& b) { return compare(a, b) <= 0; }
    friend bool operator>(const BigUInt& a, const BigUInt& b) { return compare(a, b) > 0; }
    friend bool operator>=(const BigUInt& a, const BigUInt& b) { return compare(a, b) >= 0; }

    friend std::ostream& operator<<(std::ostream& os, const BigUInt& x) { return os << x.to_string(); }

private:
    std::vector<std::uint32_t> limbs_;

    void trim() {
        while (!limbs_.empty() && limbs_.back() == 0) limbs_.pop_back();
    }
};

}  // namespace combi
