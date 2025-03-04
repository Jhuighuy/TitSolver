# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

from collections.abc import Hashable
from typing import Protocol


class RichComparable(Protocol):
    """Rich comparison protocol."""

    def __eq__(self, other: object, /) -> bool: ...
    def __ne__(self, other: object, /) -> bool: ...
    def __lt__(self, other: object, /) -> bool: ...
    def __le__(self, other: object, /) -> bool: ...
    def __gt__(self, other: object, /) -> bool: ...
    def __ge__(self, other: object, /) -> bool: ...


class TitEnum(Hashable, RichComparable):
    """Enumeration protocol used by native extension modules."""

    name: str
    value: int

    # pylint: disable=unused-argument
    def __init__(self, value: int) -> None: ...
    def __str__(self) -> str: ...
