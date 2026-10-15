#include <nanobind/nanobind.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/vector.h>

#include <cstdint>
#include <utility>
#include <vector>

#include "sample/sieve.hpp"
#include "sample/tsp.hpp"

namespace nb = nanobind;
using namespace nb::literals;

NB_MODULE(_core, m) {
    m.doc() = "C++ implementation of the Sieve of Eratosthenes and an exact TSP solver";

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

    // TSP が扱えるノード数の上限（デポ含む）
    m.attr("MAX_TSP_NODES") = sample::kMaxTspNodes;

    // 型チェックや Python 的なインターフェースは src/sample/tsp.py が担当する。
    // ここは C++ の TspModel をそのまま公開するだけにしている。
    nb::class_<sample::TspModel>(m, "_TspModel")
        .def(nb::init<int, int>(), "num_nodes"_a, "depot"_a = 0)
        .def_prop_ro("num_nodes", &sample::TspModel::num_nodes)
        .def_prop_ro("depot", &sample::TspModel::depot)
        .def("set_depot", &sample::TspModel::set_depot, "depot"_a)
        .def("set_distance", &sample::TspModel::set_distance, "from_node"_a, "to_node"_a,
             "distance"_a)
        .def("set_distance_matrix", &sample::TspModel::set_distance_matrix, "distance_matrix"_a)
        .def(
            "solve",
            [](const sample::TspModel& self) {
                sample::TspResult result;
                {
                    // 計算中は GIL を解放する
                    nb::gil_scoped_release release;
                    result = self.solve();
                }
                return std::make_pair(std::move(result.route), result.distance);
            },
            "Return (route, distance) of the optimal tour.");
}
