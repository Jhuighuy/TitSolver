# `titparaviewplugin`

This folder contains the ParaView plugin for Tit Solver.

The plugin is written in Python and uses the
[ParaView Python API](https://www.paraview.org/paraview-docs/nightly/python/quick-start.html).
It interacts with the our C++ code via a shared library, that is loaded at
runtime with `ctypes`. This way we can avoid problems with the version mismatch
between ParaView's Python interpreter and our Python interpreter.
