#pragma once

#include <cstdint>
#include <vector>

namespace sample {

// エラトステネスのふるいで n 以下の素数を昇順で返す。
// n < 0 のときは std::invalid_argument を投げる。
std::vector<std::int64_t> sieve(std::int64_t n);

}  // namespace sample
