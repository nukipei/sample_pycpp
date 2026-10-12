import pytest

from sample import sieve


def naive_primes(n: int) -> list[int]:
    """素朴な試し割りによる参照実装。"""
    return [k for k in range(2, n + 1) if all(k % d for d in range(2, int(k**0.5) + 1))]


@pytest.mark.parametrize(
    ("n", "expected"),
    [
        (0, []),
        (1, []),
        (2, [2]),
        (10, [2, 3, 5, 7]),
        (30, [2, 3, 5, 7, 11, 13, 17, 19, 23, 29]),
    ],
)
def test_small_values(n: int, expected: list[int]) -> None:
    assert sieve(n) == expected


def test_one_million() -> None:
    primes = sieve(1_000_000)
    assert len(primes) == 78498
    assert primes[-1] == 999983


@pytest.mark.parametrize("n", [-1, -100])
def test_negative_raises_value_error(n: int) -> None:
    with pytest.raises(ValueError):
        sieve(n)


@pytest.mark.parametrize("n", [2.5, "10", None, [10]])
def test_non_int_raises_type_error(n: object) -> None:
    with pytest.raises(TypeError):
        sieve(n)  # type: ignore[arg-type]


def test_returns_list() -> None:
    assert isinstance(sieve(10), list)


@pytest.mark.parametrize("n", range(201))
def test_matches_naive(n: int) -> None:
    assert sieve(n) == naive_primes(n)
