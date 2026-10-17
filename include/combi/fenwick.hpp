// Дерево Фенвика (Fenwick tree, binary indexed tree) над множеством «свободных»
// чисел 0..n-1. Все операции — за O(log n):
//   count_less(x) — сколько свободных чисел меньше x;
//   erase(x)      — пометить x занятым;
//   kth(k)        — k-е по величине свободное число (k считается с нуля).
// Это ровно то, что нужно для перехода «перестановка <-> код Лемера».
#pragma once

#include <cstdint>
#include <vector>

namespace combi {

class FenwickSet {
public:
    // Изначально свободны все числа 0..n-1. Построение за O(n).
    explicit FenwickSet(std::uint32_t n) : n_(n), tree_(static_cast<std::size_t>(n) + 1, 0) {
        // В ячейке i хранится количество свободных на отрезке длины lowbit(i).
        for (std::uint32_t i = 1; i <= n_; ++i) tree_[i] = lowbit(i);
        top_ = 1;
        while (top_ <= n_ / 2) top_ *= 2;
        if (n_ == 0) top_ = 0;
    }

    std::uint32_t count_less(std::uint32_t x) const {
        std::uint32_t s = 0;
        for (std::uint32_t i = x; i > 0; i -= lowbit(i)) s += tree_[i];
        return s;
    }

    void erase(std::uint32_t x) {
        for (std::uint32_t i = x + 1; i <= n_; i += lowbit(i)) --tree_[i];
    }

    // Спуск по степеням двойки: находим самую левую позицию, где префиксная
    // сумма свободных становится больше k.
    std::uint32_t kth(std::uint32_t k) const {
        std::uint32_t pos = 0;
        for (std::uint32_t step = top_; step > 0; step >>= 1) {
            if (pos + step <= n_ && tree_[pos + step] <= k) {
                pos += step;
                k -= tree_[pos];
            }
        }
        return pos;
    }

private:
    static std::uint32_t lowbit(std::uint32_t i) { return i & (0u - i); }

    std::uint32_t n_;
    std::uint32_t top_ = 0;
    std::vector<std::uint32_t> tree_;
};

}  // namespace combi
