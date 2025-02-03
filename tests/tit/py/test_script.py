import sys
import os

sys.path.append(os.environ["INSTALL_DIR"] + "/private/lib")

import test_module  # type: ignore

# Constants.
print(test_module.PI)

# Very basic functionality.
print(test_module.hello)
print(dir(test_module.hello))
print(test_module.hello())

# Various argument combinations.
print(test_module.test_func(1, 2, 3, 4))
print(test_module.test_func(1, 2, 3))
print(test_module.test_func(1, 2, a=3))
print(test_module.test_func(1, 2))
try:
    test_module.test_func(1)  # type: ignore
except TypeError as e:
    print(f"Expected error: {e}")
try:
    test_module.test_func(1, 2, 3, 4, 5)  # type: ignore
except TypeError as e:
    print(f"Expected error: {e}")
try:
    test_module.test_func("1", "2")  # type: ignore
except TypeError as e:
    print(f"Expected error: {e}")
try:
    test_module.test_func(1, 2, p=3)  # type: ignore
except TypeError as e:
    print(f"Expected error: {e}")

# Exception test.
try:
    test_module.throw()
except AssertionError as e:
    print(f"Expected exception: {e}")

# Class test.
t = test_module.TestClass(123)
s, tt = t.hello_w()
print(s, flush=True)
print(tt is t, flush=True)
print(tt.hello_w, flush=True)
print(f"get a = {tt.a}", flush=True)
t.a = 228
print(tt.a, flush=True)
