// Чтобы русский текст нормально печатался в консоли Windows.
// В Linux и macOS ничего не делает. Нужен только исполняемым файлам (tests, bench, demo).
#pragma once

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX  // иначе windows.h ломает std::min / std::max
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN  // не тянуть лишние заголовки с макросами
#endif
#include <windows.h>
#endif

namespace combi {

inline void setup_console_utf8() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);  // CP_UTF8
#endif
}

}  // namespace combi
