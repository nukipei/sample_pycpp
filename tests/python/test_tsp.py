import itertools
import random

import pytest

from sample import MAX_TSP_NODES, TspModel

# OR-Tools の TSP チュートリアルと同じ 13 都市の距離行列（最適解は 7293）
ORTOOLS_MATRIX = [
    [0, 2451, 713, 1018, 1631, 1374, 2408, 213, 2571, 875, 1420, 2145, 1972],
    [2451, 0, 1745, 1524, 831, 1240, 959, 2596, 403, 1589, 1374, 357, 579],
    [713, 1745, 0, 355, 920, 803, 1737, 851, 1858, 262, 940, 1453, 1260],
    [1018, 1524, 355, 0, 700, 862, 1395, 1123, 1584, 466, 1056, 1280, 987],
    [1631, 831, 920, 700, 0, 663, 1021, 1769, 949, 796, 879, 586, 371],
    [1374, 1240, 803, 862, 663, 0, 1681, 1551, 1765, 547, 225, 887, 999],
    [2408, 959, 1737, 1395, 1021, 1681, 0, 2493, 678, 1724, 1891, 1114, 701],
    [213, 2596, 851, 1123, 1769, 1551, 2493, 0, 2699, 1038, 1605, 2300, 2099],
    [2571, 403, 1858, 1584, 949, 1765, 678, 2699, 0, 1744, 1645, 653, 600],
    [875, 1589, 262, 466, 796, 547, 1724, 1038, 1744, 0, 679, 1272, 1162],
    [1420, 1374, 940, 1056, 879, 225, 1891, 1605, 1645, 679, 0, 1017, 1200],
    [2145, 357, 1453, 1280, 586, 887, 1114, 2300, 653, 1272, 1017, 0, 504],
    [1972, 579, 1260, 987, 371, 999, 701, 2099, 600, 1162, 1200, 504, 0],
]


def tour_length(matrix: list[list[int]], route: list[int]) -> int:
    return sum(matrix[a][b] for a, b in itertools.pairwise(route))


def brute_force(matrix: list[list[int]], depot: int) -> int:
    others = [v for v in range(len(matrix)) if v != depot]
    return min(
        tour_length(matrix, [depot, *perm, depot])
        for perm in itertools.permutations(others)
    )


def solve(matrix: list[list[int]], depot: int = 0):
    model = TspModel(len(matrix), depot)
    model.set_distance_matrix(matrix)
    return model.solve()


def test_ortools_example() -> None:
    sol = solve(ORTOOLS_MATRIX)
    assert sol.distance == 7293
    assert sol.route[0] == sol.route[-1] == 0
    assert sorted(sol.route[:-1]) == list(range(13))
    assert tour_length(ORTOOLS_MATRIX, sol.route) == sol.distance


@pytest.mark.parametrize("seed", range(20))
def test_matches_brute_force_asymmetric(seed: int) -> None:
    rng = random.Random(seed)
    n = rng.randint(2, 7)
    matrix = [[rng.randint(0, 100) for _ in range(n)] for _ in range(n)]
    depot = rng.randrange(n)
    sol = solve(matrix, depot)
    assert sol.distance == brute_force(matrix, depot)
    assert sol.route[0] == sol.route[-1] == depot
    assert sorted(sol.route[:-1]) == list(range(n))
    assert tour_length(matrix, sol.route) == sol.distance


def test_single_node() -> None:
    sol = TspModel(1).solve()
    assert sol.route == [0, 0]
    assert sol.distance == 0


def test_max_nodes_solves() -> None:
    n = MAX_TSP_NODES
    # 0→1→...→n-1→0 と進むのが最適になる行列
    matrix = [[1 if (j - i) % n == 1 else 50 for j in range(n)] for i in range(n)]
    sol = solve(matrix)
    assert sol.distance == n
    assert sol.route == [*range(n), 0]


def test_set_distance_and_callback() -> None:
    sub = [r[:4] for r in ORTOOLS_MATRIX[:4]]
    expected = solve(sub)

    model = TspModel(4)
    for i in range(4):
        for j in range(4):
            if i != j:
                model.set_distance(i, j, sub[i][j])
    assert model.solve() == expected

    other = TspModel(4)
    other.set_distance_callback(lambda i, j: sub[i][j])
    assert other.solve() == expected


def test_too_many_nodes() -> None:
    with pytest.raises(ValueError):
        TspModel(MAX_TSP_NODES + 1)


@pytest.mark.parametrize("n", [0, -1])
def test_invalid_num_nodes(n: int) -> None:
    with pytest.raises(ValueError):
        TspModel(n)


@pytest.mark.parametrize("depot", [-1, 3])
def test_depot_out_of_range(depot: int) -> None:
    with pytest.raises(ValueError):
        TspModel(3, depot)


def test_invalid_distances() -> None:
    model = TspModel(3)
    with pytest.raises(ValueError):
        model.set_distance(0, 1, -1)
    with pytest.raises(ValueError):
        model.set_distance(0, 3, 1)
    with pytest.raises(ValueError):
        model.set_distance_matrix([[0, 1], [1, 0]])
    with pytest.raises(ValueError):
        model.set_distance_matrix([[0, 1, 1], [1, 0, 1], [1, 1, -5]])


def test_failed_set_matrix_keeps_previous() -> None:
    model = TspModel(2)
    model.set_distance_matrix([[0, 5], [7, 0]])
    with pytest.raises(ValueError):
        model.set_distance_matrix([[0, 1], [1, -1]])
    assert model.solve().distance == 12


@pytest.mark.parametrize("bad", [1.5, "1", None, True])
def test_type_errors(bad: object) -> None:
    model = TspModel(3)
    with pytest.raises(TypeError):
        model.set_distance(0, 1, bad)  # type: ignore[arg-type]
    with pytest.raises(TypeError):
        TspModel(bad)  # type: ignore[arg-type]
