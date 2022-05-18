import functools
from time import time_ns
import taichi as ti

class ScopedTimeCounter:
    def __init__(self, name, enable):
        self._name = name
        self._enable = enable
        self._start = 0

    def __enter__(self):
        if self._enable:
            self._start = time_ns()

    def __exit__(self, exc_type, exc_val, exc_tb):
        # Note: unsafe
        if self._enable:
            print(f"{self._name}: {(time_ns() - self._start) * 1e-6}ms")


def kernel_with_printing_1st_time(func):
    func._1st_run = True

    @functools.wraps(func)
    def wrapped(*args, **kwargs):
        with ScopedTimeCounter(f"1st run {func.__qualname__} spent", func._1st_run):
            func._1st_run = False
            return ti.kernel(func)(*args, **kwargs)

    return wrapped


__all__ = [
    'kernel_with_printing_1st_time'
]
