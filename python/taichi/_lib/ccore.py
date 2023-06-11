import os
import sys
import ctypes

from taichi._lib.utils import package_root


def _load_dll(path):
    try:
        if (
            sys.version_info[0] > 3
            or sys.version_info[0] == 3
            and sys.version_info[1] >= 8
        ):
            dll = ctypes.CDLL(path, winmode=0)
        else:
            dll = ctypes.CDLL(path)
    except OSError:
        return None
    return dll


def load_core_exports_dll():
    bin_path = os.path.join(package_root, "_lib", "core_exports", "bin")
    if os.name == "nt":
        if (
            sys.version_info[0] > 3
            or sys.version_info[0] == 3
            and sys.version_info[1] >= 8
        ):
            os.add_dll_directory(bin_path)
        else:
            os.environ["PATH"] = bin_path + os.pathsep + os.environ["PATH"]
        dll_path = os.path.join(bin_path, "taichi_core_exports.dll")
    elif sys.platform == "darwin":
        dll_path = os.path.join(bin_path, "libtaichi_core_exports.dylib")
    else:
        dll_path = os.path.join(bin_path, "taichi_core_exports.so")

    return _load_dll(dll_path)


# NOTE: Temporary solution
class TaichiCCore:
    def __init__(self) -> None:
        self._dll = load_core_exports_dll()
        if self._dll is None:
            raise RuntimeError("Cannot load taichi_core_exports.dll")

        # int ticore_hello_world(const char *extra_msg);
        self._dll.ticore_hello_world.argtypes = [ctypes.c_char_p]
        self._dll.ticore_hello_world.restype = ctypes.c_int

        # int ticore_make_launch_context(const void *p_kernel, void **ret_ctx);
        self._dll.ticore_make_launch_context.argtypes = [
            ctypes.c_void_p,
            ctypes.POINTER(ctypes.c_void_p),
        ]
        self._dll.ticore_make_launch_context.restype = ctypes.c_int

        # int ticore_make_launch_context_EMPTY(const void *p_kernel, void **ret_ctx);
        self._dll.ticore_make_launch_context_EMPTY.argtypes = [
            ctypes.c_void_p,
            ctypes.POINTER(ctypes.c_void_p),
        ]
        self._dll.ticore_make_launch_context_EMPTY.restype = ctypes.c_int

        # int ticore_launch_context_set_arg_int(void *p_ctx, int arg_id, int64_t d);
        self._dll.ticore_launch_context_set_arg_int.argtypes = [
            ctypes.c_void_p,
            ctypes.c_int,
            ctypes.c_int64,
        ]
        self._dll.ticore_launch_context_set_arg_int.restype = ctypes.c_int

        # int ticore_launch_context_set_arg_int_EMPTY(void *p_ctx, int arg_id, int64_t d);
        self._dll.ticore_launch_context_set_arg_int_EMPTY.argtypes = [
            ctypes.c_void_p,
            ctypes.c_int,
            ctypes.c_int64,
        ]
        self._dll.ticore_launch_context_set_arg_int_EMPTY.restype = ctypes.c_int

    def hello_world(self, extra_msg):
        return self._dll.ticore_hello_world(extra_msg.encode("utf-8"))

    def make_launch_context(self, kernel):
        p_ctx = ctypes.c_void_p()
        ret = self._dll.ticore_make_launch_context(kernel, ctypes.byref(p_ctx))
        if ret != 0:
            raise RuntimeError("Failed to make launch context")
        return p_ctx.value

    def make_launch_context_EMPTY(self, kernel):
        p_ctx = ctypes.c_void_p()
        ret = self._dll.ticore_make_launch_context_EMPTY(kernel, ctypes.byref(p_ctx))
        if ret != 0:
            raise RuntimeError("Failed to make launch context EMPTY")
        return p_ctx.value

    def launch_context_set_arg_int(self, p_ctx, arg_id, d):
        ret = self._dll.ticore_launch_context_set_arg_int(p_ctx, arg_id, d)
        if ret != 0:
            raise RuntimeError("Failed to set arg int")

    def launch_context_set_arg_int_EMPTY(self, p_ctx, arg_id, d):
        ret = self._dll.ticore_launch_context_set_arg_int_EMPTY(p_ctx, arg_id, d)
        if ret != 0:
            raise RuntimeError("Failed to set arg int EMPTY")


taichi_ccore = TaichiCCore()
