# This file is auto-generated by misc/exports_to_py.py
# DO NOT edit this file manually!
# To regenerate this file, run:
#     python misc/exports_to_py.py

import ctypes
from .ccore import taichi_ccore
from .utils import get_exception_to_throw_if_not_success, get_python_object_from_handle, wrap_callback_to_c


from .global_functions import get_last_error

from .global_functions import set_pytype_tp_finalize


# Class Kernel
class Kernel:
    def __init__(self, *args, handle=None, manage_handle=False):
        if handle is not None:
            self._manage_handle = manage_handle
            self._handle = handle
        else:
            self._manage_handle = True
            self._handle = self.create(*args)

    def get_handle(self):
        return self._handle

    def insert_scalar_param(self, dt, name):
        ret_param_index = ctypes.c_int()
        ret = taichi_ccore.tie_Kernel_insert_scalar_param(self.get_handle(), dt.get_handle(), name.encode("utf-8"), ctypes.byref(ret_param_index))
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex
        return (
            ret_param_index.value    
        )

    def insert_arr_param(self, dt, total_dim, ap_element_shape, name):
        ap_element_shape = (ctypes.c_int * len(ap_element_shape))(*ap_element_shape)
        ret_param_index = ctypes.c_int()
        ret = taichi_ccore.tie_Kernel_insert_arr_param(self.get_handle(), dt.get_handle(), int(total_dim), ap_element_shape, len(ap_element_shape), name.encode("utf-8"), ctypes.byref(ret_param_index))
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex
        return (
            ret_param_index.value    
        )

    def insert_ndarray_param(self, dt, ndim, name, needs_grad):
        ret_param_index = ctypes.c_int()
        ret = taichi_ccore.tie_Kernel_insert_ndarray_param(self.get_handle(), dt.get_handle(), int(ndim), name.encode("utf-8"), int(needs_grad), ctypes.byref(ret_param_index))
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex
        return (
            ret_param_index.value    
        )

    def insert_texture_param(self, total_dim, name):
        ret_param_index = ctypes.c_int()
        ret = taichi_ccore.tie_Kernel_insert_texture_param(self.get_handle(), int(total_dim), name.encode("utf-8"), ctypes.byref(ret_param_index))
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex
        return (
            ret_param_index.value    
        )

    def insert_pointer_param(self, dt, name):
        ret_param_index = ctypes.c_int()
        ret = taichi_ccore.tie_Kernel_insert_pointer_param(self.get_handle(), dt.get_handle(), name.encode("utf-8"), ctypes.byref(ret_param_index))
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex
        return (
            ret_param_index.value    
        )

    def insert_rw_texture_param(self, total_dim, format, name):
        ret_param_index = ctypes.c_int()
        ret = taichi_ccore.tie_Kernel_insert_rw_texture_param(self.get_handle(), int(total_dim), int(format), name.encode("utf-8"), ctypes.byref(ret_param_index))
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex
        return (
            ret_param_index.value    
        )

    def insert_ret(self, dt):
        ret_ret_index = ctypes.c_int()
        ret = taichi_ccore.tie_Kernel_insert_ret(self.get_handle(), dt.get_handle(), ctypes.byref(ret_ret_index))
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex
        return (
            ret_ret_index.value    
        )

    def finalize_rets(self):
        ret = taichi_ccore.tie_Kernel_finalize_rets(self.get_handle())
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex

    def finalize_params(self):
        ret = taichi_ccore.tie_Kernel_finalize_params(self.get_handle())
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex

    def ast_builder(self):
        ret_ast_builder = ctypes.c_void_p()
        ret = taichi_ccore.tie_Kernel_ast_builder(self.get_handle(), ctypes.byref(ret_ast_builder))
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex
        return (
            get_python_object_from_handle('TieASTBuilderRef', ret_ast_builder.value, manage_handle=False)    
        )

    def no_activate(self, snode):
        ret = taichi_ccore.tie_Kernel_no_activate(self.get_handle(), snode.get_handle())
        ex = get_exception_to_throw_if_not_success(ret, get_last_error)
        if ex is not None:
            raise ex



if hasattr(taichi_ccore, 'tie_Kernel_destroy'):
    destroy_fn_addr = ctypes.addressof(taichi_ccore.tie_Kernel_destroy)
    destroy_fn_addr = ctypes.c_void_p.from_address(destroy_fn_addr).value
    Kernel._tie_api_tp_finalize = destroy_fn_addr
    set_pytype_tp_finalize(id(Kernel))


__all__ = ['Kernel']
