#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

#include <cstdint>
#include <vector>

#include "sample/sieve.hpp"

namespace nb = nanobind;
using namespace nb::literals;

NB_MODULE(_core, m) {
    m.doc() = "C++ implementation of the Sieve of Eratosthenes";

    m.def(
        "sieve",
        [](std::int64_t n) {
            std::vector<std::int64_t> result;
            {
                // 計算中は GIL を解放する
                nb::gil_scoped_release release;
                result = sample::sieve(n);
            }
            return result;
        },
        "n"_a,
        "Return all primes less than or equal to n in ascending order.\n\n"
        "Uses the Sieve of Eratosthenes implemented in C++.\n\n"
        "Raises:\n"
        "    ValueError: if n is negative.\n"
        "    TypeError: if n is not an int.");
}
