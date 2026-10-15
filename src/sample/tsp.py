"""Exact traveling salesman solver (bit DP) with an OR-Tools-like model API."""

from __future__ import annotations

from collections.abc import Callable, Sequence
from dataclasses import dataclass

from ._core import MAX_TSP_NODES, _TspModel

__all__ = ["MAX_TSP_NODES", "TspModel", "TspSolution"]


def _check_int(value: object, name: str) -> int:
    # bool は int のサブクラスなので明示的に弾く
    if isinstance(value, bool) or not isinstance(value, int):
        raise TypeError(f"{name} must be int, not {type(value).__name__}")
    return value


@dataclass(frozen=True)
class TspSolution:
    """Result of :meth:`TspModel.solve`.

    route: node visiting order; starts and ends at the depot.
    distance: total distance of the route.
    """

    route: list[int]
    distance: int


class TspModel:
    """Traveling salesman problem model solved exactly with bit DP.

    Usage:
        model = TspModel(num_nodes=4, depot=0)
        model.set_distance_matrix(matrix)
        solution = model.solve()

    Distances that are not set default to 0. At most ``MAX_TSP_NODES`` nodes
    (including the depot) are supported.

    Raises:
        TypeError: if an argument is not an int (bool is also rejected).
        ValueError: if a value is out of range or the matrix has a wrong shape.
    """

    def __init__(self, num_nodes: int, depot: int = 0) -> None:
        self._model = _TspModel(
            _check_int(num_nodes, "num_nodes"), _check_int(depot, "depot")
        )
        self._callback: Callable[[int, int], int] | None = None

    @property
    def num_nodes(self) -> int:
        return self._model.num_nodes

    @property
    def depot(self) -> int:
        return self._model.depot

    def set_depot(self, depot: int) -> None:
        """Set the start/end node."""
        self._model.set_depot(_check_int(depot, "depot"))

    def set_distance(self, from_node: int, to_node: int, distance: int) -> None:
        """Set the distance from ``from_node`` to ``to_node`` (directed)."""
        self._model.set_distance(
            _check_int(from_node, "from_node"),
            _check_int(to_node, "to_node"),
            _check_int(distance, "distance"),
        )

    def set_distance_matrix(self, distance_matrix: Sequence[Sequence[int]]) -> None:
        """Set all distances at once. Must be ``num_nodes`` x ``num_nodes``."""
        matrix = [[_check_int(d, "distance") for d in row] for row in distance_matrix]
        self._model.set_distance_matrix(matrix)

    def set_distance_callback(self, callback: Callable[[int, int], int] | None) -> None:
        """Set a function ``(from_node, to_node) -> int`` evaluated at solve().

        When set, it takes precedence over distances set by the other methods.
        Pass None to clear it.
        """
        if callback is not None and not callable(callback):
            raise TypeError("callback must be callable or None")
        self._callback = callback

    def solve(self) -> TspSolution:
        """Return the optimal tour starting and ending at the depot."""
        model = self._model
        if self._callback is not None:
            # コールバックの結果は solve 用の一時モデルにだけ反映し、設定済みの距離は変えない
            n = model.num_nodes
            model = _TspModel(n, model.depot)
            for i in range(n):
                for j in range(n):
                    if i != j:
                        d = _check_int(self._callback(i, j), "distance")
                        model.set_distance(i, j, d)
        route, distance = model.solve()
        return TspSolution(route=route, distance=distance)
