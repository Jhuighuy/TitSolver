# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

from __future__ import annotations

PI = ...

# pylint: disable=unused-argument
def hello() -> str: ...
def test_func(p: int, q: int, a: int = 1, b: int = 2) -> str: ...
def throw() -> None: ...

class TestClass:
    def __init__(self, value: int) -> None: ...
    def hello_w(self) -> tuple[str, TestClass]: ...
    @property
    def a(self) -> int: ...
    @a.setter
    def a(self, value: int) -> None: ...
