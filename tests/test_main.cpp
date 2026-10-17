#include <chrono>
#include <exception>
#include <iostream>
#include <string>

#include "check.hpp"
#include "combi/console.hpp"

void test_biguint();
void test_permutations();
void test_combinations();

namespace {

void run(const char* title, void (*test)()) {
    const long long failed_before = check::stats().failed;
    const auto t0 = std::chrono::steady_clock::now();
    try {
        test();
    } catch (const std::exception& e) {  // ошибка вне CHECK: считаем провалом и идём дальше
        check::fail(title, 0, std::string("неожиданное исключение: ") + e.what());
    }
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0);
    const bool ok = check::stats().failed == failed_before;
    std::cout << (ok ? "[ OK ] " : "[FAIL] ") << title << "  (" << ms.count() << " мс)\n";
}

}  // namespace

int main() {
    combi::setup_console_utf8();
    std::cout << "Тесты combi-rank-select\n";
    run("BigUInt: длинная арифметика", test_biguint);
    run("Перестановки: lex (наивно, Фенвик), Myrvold-Ruskey", test_permutations);
    run("Сочетания: lex и colex, uint64 и BigUInt", test_combinations);
    const auto& s = check::stats();
    std::cout << "\nПроверок пройдено: " << s.passed << ", провалено: " << s.failed << "\n";
    return s.failed == 0 ? 0 : 1;
}
