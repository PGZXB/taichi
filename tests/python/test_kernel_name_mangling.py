import functools
import inspect
import pytest
from taichi.lang.enums import Layout
from taichi.lang.kernel_impl import (TaichiKernelArgumentListKeyGetter,
                                     _mangle_kernel_name)

import taichi as ti
from taichi import types
from tests import test_utils

module_prefix = ''.join([f"{len(e)}{e}" for e in __name__.split('.')])
supported_archs_taichi_ndarray = [ti.cpu, ti.cuda, ti.opengl, ti.vulkan]
grad_val_list = [False, True]


def mangled_kernel_name_getter(func):
    argument_annotations = []
    sig = inspect.signature(func)
    params = sig.parameters
    arg_names = params.keys()
    for _, arg_name in enumerate(arg_names):
        argument_annotations.append(params[arg_name].annotation)

    arg_list_key_getter = TaichiKernelArgumentListKeyGetter(
        annotations=argument_annotations)
    full_pkg_path = inspect.getmodule(func).__name__
    qualname = func.__qualname__

    @functools.wraps(func)
    def wrapped(*args, **kwargs):
        assert len(kwargs) == 0
        mangled_kernel_name = _mangle_kernel_name(
            full_pkg_path, qualname, arg_list_key_getter.lookup_key(args),
            False)
        mangled_grad_kernel_name = _mangle_kernel_name(
            full_pkg_path, qualname, arg_list_key_getter.lookup_key(args),
            True)
        return mangled_kernel_name, mangled_grad_kernel_name

    return wrapped


def set_prefix_atrr_for_func(func):
    outer_func_name = func.__name__
    locals_str = '_locals_'  # <locals>
    prefix_without_grad = module_prefix + f"{len(outer_func_name)}{outer_func_name}{len(locals_str)}{locals_str}"
    func.prefixes = ["__n" + prefix_without_grad, "__g" + prefix_without_grad]

    @functools.wraps(func)
    def wrapped(*args, **kwargs):
        func(*args, **kwargs)

    return wrapped


@pytest.mark.parametrize('grad', grad_val_list)
@test_utils.test()
@set_prefix_atrr_for_func
def test_kernel_without_params(grad):
    prefix = test_kernel_without_params.prefixes[grad]

    @mangled_kernel_name_getter
    def empty_func():
        pass

    assert prefix + '10empty_func' == empty_func()[grad]


@pytest.mark.parametrize('grad', grad_val_list)
@test_utils.test()
@set_prefix_atrr_for_func
def test_kernel_with_primitive_params(grad):
    prefix = test_kernel_with_primitive_params.prefixes[grad]

    @mangled_kernel_name_getter
    def func_i(a: ti.i64, b: ti.i32, c: ti.i16, d: ti.i8, e: int):
        pass

    @mangled_kernel_name_getter
    def func_u(a: ti.u64, b: ti.u32, c: ti.u16, d: ti.u8):
        pass

    @mangled_kernel_name_getter
    def func_f(a: ti.f64, b: ti.f32, c: ti.f16, d: float):
        pass

    assert prefix + '6func_io3i64o3i32o3i16o2i8o3int' == func_i(0, 0, 0, 0,
                                                                0)[grad]
    assert prefix + '6func_uo3u64o3u32o3u16o2u8' == func_u(0, 0, 0, 0)[grad]
    assert prefix + '6func_fo3f64o3f32o3f16o5float' == func_f(
        0.0, 0.0, 0.0, 0.0)[grad]


@pytest.mark.parametrize('grad', grad_val_list)
@test_utils.test()
@set_prefix_atrr_for_func
def test_kernel_with_matrix_params(grad):
    prefix = test_kernel_with_matrix_params.prefixes[grad]
    mat_anno = types.matrix(2, 2, ti.i32)
    mat = ti.Matrix([[0, 0], [0, 0]])

    @mangled_kernel_name_getter
    def func(a: mat_anno, b: mat_anno):
        pass

    assert prefix + '4funco3mato3mat' == func(mat, mat)[grad]


@pytest.mark.parametrize('grad', grad_val_list)
@test_utils.test()
@set_prefix_atrr_for_func
def test_kernel_with_sparse_matrix_params(grad):
    prefix = test_kernel_with_sparse_matrix_params.prefixes[grad]
    sparse_mat_anno = types.sparse_matrix_builder()
    sparse_mat = ti.linalg.SparseMatrixBuilder(2, 4, max_num_triplets=8)

    @mangled_kernel_name_getter
    def func(a: sparse_mat_anno, b: sparse_mat_anno):
        pass

    assert prefix + '4funco3smbo3smb' == func(sparse_mat, sparse_mat)[grad]


@pytest.mark.parametrize('grad', grad_val_list)
@test_utils.test(arch=supported_archs_taichi_ndarray)
@set_prefix_atrr_for_func
def test_kernel_with_arr_params(grad):
    prefix = test_kernel_with_arr_params.prefixes[grad]
    arr_anno = types.any_arr()

    vals = (
        (ti.ndarray(ti.i32, ()), 'a3i320sA'),
        (ti.ndarray(ti.i32, (1, )), 'a3i321sA'),
        (ti.ndarray(ti.i32, (2, 3)), 'a3i322sA'),
        (ti.ndarray(ti.i32, (4, 5, 6)), 'a3i323sA'),
        (ti.Vector.ndarray(12, ti.i32, (), Layout.AOS), 'a3i321s12A'),
        (ti.Vector.ndarray(3, ti.i32, (1, ), Layout.SOA), 'a3i322s3S'),
        (ti.Vector.ndarray(4, ti.i32, (2, 3), Layout.AOS), 'a3i323s4A'),
        (ti.Vector.ndarray(5, ti.i32, (5, 7, 2), Layout.SOA), 'a3i324s5S'),
        (ti.Matrix.ndarray(4, 3, ti.i32, (), Layout.AOS), 'a3i322s4x3A'),
        (ti.Matrix.ndarray(8, 3, ti.i32, (2, ), Layout.SOA), 'a3i323s8x3S'),
        (ti.Matrix.ndarray(2, 12, ti.i32, (2, 2), Layout.SOA), 'a3i324s2x12S'),
        (ti.Matrix.ndarray(2, 3, ti.i32, (2, 2, 2),
                           Layout.AOS), 'a3i325s2x3A'),
    )

    @mangled_kernel_name_getter
    def func_1_arr(a: arr_anno):
        pass

    @mangled_kernel_name_getter
    def func_2_arr(a: arr_anno, b: arr_anno):
        pass

    @mangled_kernel_name_getter
    def func_3_arr(a: arr_anno, b: arr_anno, c: arr_anno):
        pass

    temp_prefix = prefix + '10func_1_arr'
    for a, a_key in vals:
        assert temp_prefix + a_key == func_1_arr(a)[grad]

    temp_prefix = prefix + '10func_2_arr'
    for a, a_key in vals:
        for b, b_key in vals:
            assert temp_prefix + f"{a_key}{b_key}" == func_2_arr(a, b)[grad]

    temp_prefix = prefix + '10func_3_arr'
    for a, a_key in vals:
        for b, b_key in vals:
            for c, c_key in vals:
                assert temp_prefix + f"{a_key}{b_key}{c_key}" == func_3_arr(
                    a, b, c)[grad]
