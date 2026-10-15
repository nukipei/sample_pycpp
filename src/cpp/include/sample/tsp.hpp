#pragma once

#include <cstdint>
#include <vector>

namespace sample {

// 扱えるノード数（デポを含む）の上限。bit DP のメモリが 2^(n-1) * (n-1) に比例するため制限している。
constexpr int kMaxTspNodes = 16;

// 距離行列の要素の上限。経路の合計がオーバーフローしないようにするための制約。
constexpr std::int64_t kMaxTspDistance = 1'000'000'000'000;  // 1e12

struct TspResult {
    // depot から出発して全ノードを 1 回ずつ訪れ、depot に戻る順序（先頭と末尾はともに depot）。
    std::vector<int> route;
    // route の合計距離。
    std::int64_t distance;
};

// bit DP による厳密な巡回セールスマン問題（TSP）の解法。
// distance_matrix[i][j] は i から j への距離（非対称でもよい。対角成分は無視される）。
// 以下の場合は std::invalid_argument を投げる。
//   - ノード数が 0、または kMaxTspNodes を超える
//   - 正方行列でない
//   - 要素が負、または kMaxTspDistance を超える
//   - depot が範囲外
TspResult solve_tsp(const std::vector<std::vector<std::int64_t>>& distance_matrix, int depot = 0);

// OR-Tools のように「モデルを作り、設定し、solve で求解する」ための入れ物。
// 設定されなかった距離は 0。不正な引数は std::invalid_argument を投げる。
class TspModel {
public:
    // num_nodes は 1 以上 kMaxTspNodes 以下、depot は [0, num_nodes)。
    explicit TspModel(int num_nodes, int depot = 0);

    int num_nodes() const { return num_nodes_; }
    int depot() const { return depot_; }

    void set_depot(int depot);
    // from_node から to_node への距離（有向）を設定する。
    void set_distance(int from_node, int to_node, std::int64_t distance);
    // num_nodes x num_nodes の行列で一括設定する。失敗時は既存の設定を変更しない。
    void set_distance_matrix(const std::vector<std::vector<std::int64_t>>& distance_matrix);

    // 最適な巡回路を返す。
    TspResult solve() const;

private:
    int num_nodes_;
    int depot_ = 0;
    std::vector<std::vector<std::int64_t>> matrix_;
};

}  // namespace sample
