#!/usr/bin/env python3
"""Эталонные значения для тестов C++.

Независимая реализация на Python (встроенная длинная арифметика), написанная
«по определению», а не теми же формулами, что в C++. Сначала скрипт сам себя
проверяет перебором на маленьких n (itertools), потом печатает числа, которые
вставлены в tests/*.cpp.

Запуск:  python3 tools/reference.py
"""
from itertools import combinations, permutations
from math import comb, factorial


# ---------- Перестановки ----------

def perm_rank_lex(p):
    """Лексикографический номер: сколько перестановок меньше p (код Лемера)."""
    n = len(p)
    r = 0
    for i in range(n):
        smaller_right = sum(1 for j in range(i + 1, n) if p[j] < p[i])
        r += smaller_right * factorial(n - 1 - i)
    return r


def perm_rank_mr(p):
    """Myrvold–Ruskey (2001), дословно по псевдокоду статьи (рекурсивно)."""
    pi = list(p)
    inv = [0] * len(pi)
    for i, v in enumerate(pi):
        inv[v] = i

    def rank(n):
        if n == 1:
            return 0
        s = pi[n - 1]
        a, b = n - 1, inv[n - 1]
        pi[a], pi[b] = pi[b], pi[a]
        inv[s], inv[n - 1] = inv[n - 1], inv[s]
        return s + n * rank(n - 1)

    return rank(len(pi)) if pi else 0


def perm_unrank_mr(r, n):
    pi = list(range(n))
    for m in range(n, 0, -1):
        d = r % m
        r //= m
        pi[m - 1], pi[d] = pi[d], pi[m - 1]
    return pi


# ---------- Сочетания ----------

def comb_rank_lex(c, n):
    """Прямой подсчёт: для каждой позиции считаем сочетания с тем же префиксом
    и меньшим очередным элементом (без формулы двойственности)."""
    k = len(c)
    r = 0
    prev = -1
    for i, ci in enumerate(c):
        for v in range(prev + 1, ci):
            r += comb(n - 1 - v, k - 1 - i)
        prev = ci
    return r


def comb_rank_colex(c):
    """Прямой подсчёт colex: идём от наибольшего элемента; для позиции i
    считаем варианты с меньшим i-м элементом v (оставшиеся i элементов < v)."""
    r = 0
    for i, ci in enumerate(c):
        for v in range(i, ci):
            r += comb(v, i)
    return r


def colex_key(c):
    return tuple(reversed(c))


# ---------- Самопроверка перебором ----------

def self_check():
    for n in range(0, 8):
        for idx, p in enumerate(permutations(range(n))):  # itertools даёт лекс. порядок
            assert perm_rank_lex(p) == idx
        seen = set()
        for p in permutations(range(n)):
            r = perm_rank_mr(p)
            assert 0 <= r < factorial(n) and r not in seen
            seen.add(r)
            assert perm_unrank_mr(r, n) == list(p)
    for n in range(0, 11):
        for k in range(0, n + 1):
            lex = list(combinations(range(n), k))  # лекс. порядок
            for idx, c in enumerate(lex):
                assert comb_rank_lex(c, n) == idx
            for idx, c in enumerate(sorted(lex, key=colex_key)):
                assert comb_rank_colex(c) == idx
    print("самопроверка перебором: OK")


def main():
    self_check()
    print("100! =", factorial(100))
    print("C(100, 50) =", comb(100, 50))
    print("C(1000, 500) =", comb(1000, 500))

    n = 50
    p = [(7 * i + 3) % n for i in range(n)]
    assert sorted(p) == list(range(n))
    print("перестановка n=50, p_i=(7i+3) mod 50:")
    print("  lex rank =", perm_rank_lex(p))
    r_mr = perm_rank_mr(p)
    assert perm_unrank_mr(r_mr, n) == p
    print("  MR rank  =", r_mr)

    n, k = 100, 30
    c = [3 * i + 1 for i in range(k)]
    print("сочетание n=100, k=30, c_i=3i+1:")
    print("  lex rank   =", comb_rank_lex(c, n))
    print("  colex rank =", comb_rank_colex(c))


if __name__ == "__main__":
    main()
