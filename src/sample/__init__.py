"""Sieve of Eratosthenes implemented in C++."""

from ._core import sieve as _sieve
from .tsp import MAX_TSP_NODES, TspModel, TspSolution

__all__ = ["MAX_TSP_NODES", "TspModel", "TspSolution", "sieve"]


def sieve(n: int) -> list[int]:
    """Return all primes less than or equal to n in ascending order.

    Uses the Sieve of Eratosthenes implemented in C++.

    Raises:
        ValueError: if n is negative.
        TypeError: if n is not an int (bool is also rejected).
    """
    # bool は int のサブクラスなので明示的に弾く
    if isinstance(n, bool) or not isinstance(n, int):
        raise TypeError(f"n must be int, not {type(n).__name__}")
    if n < 0:
        raise ValueError(f"n must be non-negative, got {n}")
    return _sieve(n)
