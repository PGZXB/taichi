# This file is auto-generated by misc/exports_to_py.py
# DO NOT edit this file manually!
# To regenerate this file, run:
#     python misc/exports_to_py.py

import ctypes
from .ccore import taichi_ccore
from .utils import get_exception_to_throw_if_not_success, get_python_object_from_handle, wrap_callback_to_c


from .global_functions import get_last_error

from .global_functions import set_pytype_tp_finalize


# Class KernelProfileTracedRecord
class KernelProfileTracedRecord:
    def __init__(self, *args, handle=None, manage_handle=False):
        if handle is not None:
            self._manage_handle = manage_handle
            self._handle = handle
        else:
            self._manage_handle = True
            self._handle = self.create(*args)

    def get_handle(self):
        return self._handle

    def get_register_per_thread(self):
        ret_register_per_thread = ctypes.c_int()
        ret = taichi_ccore.tie_KernelProfileTracedRecord_get_register_per_thread(self.get_handle(), ctypes.byref(ret_register_per_thread))
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex
        return (
            ret_register_per_thread.value    
        )

    def get_shared_mem_per_block(self):
        ret_shared_mem_per_block = ctypes.c_int()
        ret = taichi_ccore.tie_KernelProfileTracedRecord_get_shared_mem_per_block(self.get_handle(), ctypes.byref(ret_shared_mem_per_block))
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex
        return (
            ret_shared_mem_per_block.value    
        )

    def get_grid_size(self):
        ret_grid_size = ctypes.c_int()
        ret = taichi_ccore.tie_KernelProfileTracedRecord_get_grid_size(self.get_handle(), ctypes.byref(ret_grid_size))
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex
        return (
            ret_grid_size.value    
        )

    def get_block_size(self):
        ret_block_size = ctypes.c_int()
        ret = taichi_ccore.tie_KernelProfileTracedRecord_get_block_size(self.get_handle(), ctypes.byref(ret_block_size))
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex
        return (
            ret_block_size.value    
        )

    def get_active_blocks_per_multiprocessor(self):
        ret_active_blocks_per_multiprocessor = ctypes.c_int()
        ret = taichi_ccore.tie_KernelProfileTracedRecord_get_active_blocks_per_multiprocessor(self.get_handle(), ctypes.byref(ret_active_blocks_per_multiprocessor))
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex
        return (
            ret_active_blocks_per_multiprocessor.value    
        )

    def get_kernel_elapsed_time_in_ms(self):
        ret_kernel_elapsed_time_in_ms = ctypes.c_float()
        ret = taichi_ccore.tie_KernelProfileTracedRecord_get_kernel_elapsed_time_in_ms(self.get_handle(), ctypes.byref(ret_kernel_elapsed_time_in_ms))
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex
        return (
            ret_kernel_elapsed_time_in_ms.value    
        )

    def get_time_since_base(self):
        ret_time_since_base = ctypes.c_float()
        ret = taichi_ccore.tie_KernelProfileTracedRecord_get_time_since_base(self.get_handle(), ctypes.byref(ret_time_since_base))
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex
        return (
            ret_time_since_base.value    
        )

    def get_name(self):
        ret_name = ctypes.c_char_p()
        ret = taichi_ccore.tie_KernelProfileTracedRecord_get_name(self.get_handle(), ctypes.byref(ret_name))
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex
        return (
            ctypes.string_at(ret_name.value).decode("utf-8")    
        )

    def get_num_metric_values(self):
        ret_size = ctypes.c_size_t()
        ret = taichi_ccore.tie_KernelProfileTracedRecord_get_num_metric_values(self.get_handle(), ctypes.byref(ret_size))
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex
        return (
            ret_size.value    
        )

    def get_metric_value(self, index):
        ret_value = ctypes.c_float()
        ret = taichi_ccore.tie_KernelProfileTracedRecord_get_metric_value(self.get_handle(), int(index), ctypes.byref(ret_value))
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex
        return (
            ret_value.value    
        )

    register_per_thread = property(get_register_per_thread, None)
    shared_mem_per_block = property(get_shared_mem_per_block, None)
    grid_size = property(get_grid_size, None)
    block_size = property(get_block_size, None)
    active_blocks_per_multiprocessor = property(get_active_blocks_per_multiprocessor, None)
    kernel_elapsed_time_in_ms = property(get_kernel_elapsed_time_in_ms, None)
    time_since_base = property(get_time_since_base, None)
    name = property(get_name, None)
    num_metric_values = property(get_num_metric_values, None)


if hasattr(taichi_ccore, 'tie_KernelProfileTracedRecord_destroy'):
    destroy_fn_addr = ctypes.addressof(taichi_ccore.tie_KernelProfileTracedRecord_destroy)
    destroy_fn_addr = ctypes.c_void_p.from_address(destroy_fn_addr).value
    KernelProfileTracedRecord._tie_api_tp_finalize = destroy_fn_addr
    set_pytype_tp_finalize(id(KernelProfileTracedRecord))


__all__ = ['KernelProfileTracedRecord']
