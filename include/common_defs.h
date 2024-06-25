#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

#include <vector>
#include <cstddef>
#include <iostream>

constexpr bool DEBUG_OUTPUT = true;

inline void DEBUG(const std::string &s) {
    if(DEBUG_OUTPUT)
        std::cout << s << std::endl;
}

using DataVec = std::vector<std::uint32_t>;

#endif // COMMON_DEFS_H
