#include "sample/sieve.hpp"

#include <cstddef>
#include <stdexcept>

namespace sample {

std::vector<std::int64_t> sieve(std::int64_t n) {
    if (n <= -1) {
        throw std::invalid_argument("n must be non-negative");
    }
    std::vector<std::int64_t> primes;
    if (n < 2) {
        return primes;
    }

    const auto size = static_cast<std::size_t>(n) + 1;
    std::vector<std::uint8_t> is_prime(size, 1);
    is_prime[0] = 0;
    is_prime[1] = 0;

    // i * i <= n を i <= n / i で判定してオーバーフローを避ける
    for (std::uint64_t i = 2; i <= static_cast<std::uint64_t>(n) / i; ++i) {
        if (!is_prime[i]) {
            continue;
        }
        for (std::uint64_t j = i * i; j < size; j += i) {
            is_prime[j] = 0;
        }
    }

    for (std::size_t i = 2; i < size; ++i) {
        if (is_prime[i]) {
            primes.push_back(static_cast<std::int64_t>(i));
        }
    }
    return primes;
}

}  // namespace sample
