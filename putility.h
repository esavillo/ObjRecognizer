/**
 * Evan Savillo
 * Spring 2019
 * small utilities
 */

#ifndef PUTILITY_PUTILITY_H
#define PUTILITY_PUTILITY_H

#include <dirent.h>

#include <iostream>
#include <vector>

#ifndef PRINTLN
#define PRINTLN(line) std::cout << line << std::endl
#endif

#ifndef LNPRINTLN
#define LNPRINTLN(line) std::cout << std::endl << line << std::endl
#endif

namespace putility {
    std::vector<std::string> *getImageFiles(const std::string &dirname);
    // Tests whether a string ends with a given suffix (ext)
    bool endsWith(const std::string &str, const std::string &ext);
}

#endif
