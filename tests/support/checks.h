#pragma once

#include <cstdlib>
#include <iostream>
#include <string_view>

struct Checks {
    int failures = 0;

    void expect(bool condition, std::string_view description) {
        if (!condition) {
            std::cerr << "FAIL: " << description << '\n';
            failures++;
        }
    }

    int result() const {
        return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }
};
