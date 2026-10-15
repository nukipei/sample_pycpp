#include "sample/tsp.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace sample {

TspResult solve_tsp(const std::vector<std::vector<std::int64_t>>& distance_matrix, int depot) {
    const auto n = static_cast<int>(distance_matrix.size());
    if (n == 0) {
        throw std::invalid_argument("distance_matrix must not be empty");
    }
    if (n > kMaxTspNodes) {
        throw std::invalid_argument("too many nodes: " + std::to_string(n) + " (max " +
                                    std::to_string(kMaxTspNodes) + ")");
    }
    for (const auto& row : distance_matrix) {
        if (static_cast<int>(row.size()) != n) {
            throw std::invalid_argument("distance_matrix must be square");
        }
        for (const auto d : row) {
            if (d < 0 || d > kMaxTspDistance) {
                throw std::invalid_argument("distance must be in [0, " +
                                            std::to_string(kMaxTspDistance) + "]");
            }
        }
    }
    if (depot < 0 || depot >= n) {
        throw std::invalid_argument("depot is out of range");
    }
    if (n == 1) {
        return {{depot, depot}, 0};
    }

    // depot 以外の m 個のノードを 0..m-1 に詰め直し、その部分集合を bit で表す。
    std::vector<int> nodes;
    for (int v = 0; v < n; ++v) {
        if (v != depot) {
            nodes.push_back(v);
        }
    }
    const int m = n - 1;
    const std::size_t full = (std::size_t{1} << m) - 1;
    constexpr std::int64_t kInf = std::numeric_limits<std::int64_t>::max();

    // dp[mask * m + j]: depot から出発して mask の各ノードを訪れ、j で終わる最小距離。
    std::vector<std::int64_t> dp((full + 1) * m, kInf);
    // prev[mask * m + j]: その最適経路で j の 1 つ前に訪れたノード（-1 は depot）。
    std::vector<std::int8_t> prev((full + 1) * m, -1);

    for (int j = 0; j < m; ++j) {
        dp[(std::size_t{1} << j) * m + j] = distance_matrix[depot][nodes[j]];
    }
    for (std::size_t mask = 1; mask <= full; ++mask) {
        for (int j = 0; j < m; ++j) {
            const auto cur = dp[mask * m + j];
            if (!(mask >> j & 1) || cur == kInf) {
                continue;
            }
            for (int k = 0; k < m; ++k) {
                if (mask >> k & 1) {
                    continue;
                }
                const auto next_mask = mask | (std::size_t{1} << k);
                const auto cand = cur + distance_matrix[nodes[j]][nodes[k]];
                auto& slot = dp[next_mask * m + k];
                if (cand < slot) {
                    slot = cand;
                    prev[next_mask * m + k] = static_cast<std::int8_t>(j);
                }
            }
        }
    }

    // depot に戻るコストを加えて最良の終点を選ぶ。
    std::int64_t best = kInf;
    int last = 0;
    for (int j = 0; j < m; ++j) {
        const auto total = dp[full * m + j] + distance_matrix[nodes[j]][depot];
        if (total < best) {
            best = total;
            last = j;
        }
    }

    // prev をたどって経路を復元する（逆順に集めて反転）。
    TspResult result;
    result.distance = best;
    result.route.push_back(depot);
    std::size_t mask = full;
    for (int j = last; j != -1;) {
        result.route.push_back(nodes[j]);
        const int p = prev[mask * m + j];
        mask &= ~(std::size_t{1} << j);
        j = p;
    }
    std::reverse(result.route.begin() + 1, result.route.end());
    result.route.push_back(depot);
    return result;
}

TspModel::TspModel(int num_nodes, int depot) : num_nodes_(num_nodes) {
    if (num_nodes < 1 || num_nodes > kMaxTspNodes) {
        throw std::invalid_argument("num_nodes must be in [1, " + std::to_string(kMaxTspNodes) +
                                    "], got " + std::to_string(num_nodes));
    }
    matrix_.assign(num_nodes, std::vector<std::int64_t>(num_nodes, 0));
    set_depot(depot);
}

void TspModel::set_depot(int depot) {
    if (depot < 0 || depot >= num_nodes_) {
        throw std::invalid_argument("depot must be in [0, " + std::to_string(num_nodes_ - 1) +
                                    "], got " + std::to_string(depot));
    }
    depot_ = depot;
}

void TspModel::set_distance(int from_node, int to_node, std::int64_t distance) {
    if (from_node < 0 || from_node >= num_nodes_) {
        throw std::invalid_argument("from_node must be in [0, " + std::to_string(num_nodes_ - 1) +
                                    "], got " + std::to_string(from_node));
    }
    if (to_node < 0 || to_node >= num_nodes_) {
        throw std::invalid_argument("to_node must be in [0, " + std::to_string(num_nodes_ - 1) +
                                    "], got " + std::to_string(to_node));
    }
    if (distance < 0 || distance > kMaxTspDistance) {
        throw std::invalid_argument("distance must be in [0, " + std::to_string(kMaxTspDistance) +
                                    "], got " + std::to_string(distance));
    }
    matrix_[from_node][to_node] = distance;
}

void TspModel::set_distance_matrix(const std::vector<std::vector<std::int64_t>>& distance_matrix) {
    const auto n = static_cast<std::size_t>(num_nodes_);
    bool ok = distance_matrix.size() == n;
    for (const auto& row : distance_matrix) {
        ok = ok && row.size() == n;
    }
    if (!ok) {
        throw std::invalid_argument("distance_matrix must be " + std::to_string(num_nodes_) + "x" +
                                    std::to_string(num_nodes_));
    }
    // 一時オブジェクトに設定してから差し替えるので、途中で失敗しても既存の設定は壊れない。
    TspModel tmp(*this);
    for (int i = 0; i < num_nodes_; ++i) {
        for (int j = 0; j < num_nodes_; ++j) {
            tmp.set_distance(i, j, distance_matrix[i][j]);
        }
    }
    matrix_ = std::move(tmp.matrix_);
}

TspResult TspModel::solve() const { return solve_tsp(matrix_, depot_); }

}  // namespace sample
