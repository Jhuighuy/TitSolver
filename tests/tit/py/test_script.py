import sys
import os

sys.path.append(os.environ["INSTALL_DIR"] + "/private/lib")

import test_module  # type: ignore

print(test_module.hello, flush=True)
print(test_module.hello(), flush=True)
print(test_module.echo("Echo, echo!"), flush=True)
print(test_module.echo_kwargs(1, 2, a=3, b=4), flush=True)

t = test_module.TestClass(123)
s, tt = t.hello_w()
print(s, flush=True)
print(tt is t, flush=True)
print(tt.hello_w, flush=True)
print(f"get a = {tt.a}", flush=True)
t.a = 228
print(tt.a, flush=True)

try:
    test_module.TestClass("not-an-int")  # type: ignore
except Exception as e:  # pylint: disable=broad-except
    print(e, flush=True)
