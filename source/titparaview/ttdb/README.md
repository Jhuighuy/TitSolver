# `titparaview/ttdb`

This directory contains a C/Python library for interacting with BlueTit storage
files.

Since we do not have control over the specific Python interpreter used by
ParaView, we are using `ctypes` to call the C functions from Python, as it is
the most portable way to do so.
