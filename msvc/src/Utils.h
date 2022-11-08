#pragma once
#include <string>
#include <algorithm>

namespace Utils {
    template <typename T>
    std::basic_string<T> lowercase(const std::basic_string<T>& s)
    {
        std::basic_string<T> s2 = s;
        std::transform(s2.begin(), s2.end(), s2.begin(), tolower);
        return s2;
    }
}
