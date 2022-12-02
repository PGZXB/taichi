#include "taichi/program/kernel.h"

#include "taichi/rhi/cuda/cuda_driver.h"
#include "taichi/codegen/codegen.h"
#include "taichi/common/logging.h"
#include "taichi/common/task.h"
#include "taichi/ir/statements.h"
#include "taichi/ir/transforms.h"
#include "taichi/program/extension.h"
#include "taichi/program/program.h"
#include "taichi/util/action_recorder.h"

#ifdef TI_WITH_LLVM
#include "taichi/runtime/program_impls/llvm/llvm_program.h"
#endif

namespace taichi::lang {

class Function;

Kernel::Kernel(Program &program,
               const std::function<void()> &func,
               const std::string &primal_name,
               AutodiffMode autodiff_mode) {
  this->init(program, func, primal_name, autodiff_mode);
}

Kernel::Kernel(Program &program,
               const std::function<void(Kernel *)> &func,
               const std::string &primal_name,
               AutodiffMode autodiff_mode) {
  // due to #6362, we cannot write [func, this] { return func(this); }
  this->init(
      program, [&] { return func(this); }, primal_name, autodiff_mode);
}

Kernel::Kernel(Program &program,
               std::unique_ptr<IRNode> &&ir,
               const std::string &primal_name,
               AutodiffMode autodiff_mode)
    : autodiff_mode(autodiff_mode), lowered_(false) {
  this->ir = std::move(ir);
  this->program = &program;
  is_accessor = false;
  is_evaluator = false;
  compiled_ = nullptr;
  ir_is_ast_ = false;  // CHI IR
  this->ir->as<Block>()->kernel = this;

  if (autodiff_mode == AutodiffMode::kNone) {
    name = primal_name;
  } else if (autodiff_mode == AutodiffMode::kForward) {
    name = primal_name + "_forward_grad";
  } else if (autodiff_mode == AutodiffMode::kReverse) {
    name = primal_name + "_reverse_grad";
  }
}

LaunchContextBuilder Kernel::make_launch_context() {
  return LaunchContextBuilder(this);
}

LaunchContextBuilder::LaunchContextBuilder(Kernel *kernel, RuntimeContext *ctx)
    : kernel_(kernel), owned_ctx_(nullptr), ctx_(ctx) {
}

LaunchContextBuilder::LaunchContextBuilder(Kernel *kernel)
    : kernel_(kernel),
      owned_ctx_(std::make_unique<RuntimeContext>()),
      ctx_(owned_ctx_.get()) {
}

void LaunchContextBuilder::set_arg_float(int arg_id, float64 d) {
  TI_ASSERT_INFO(!kernel_->parameter_list[arg_id].is_array,
                 "Assigning scalar value to external (numpy) array argument is "
                 "not allowed.");

  ActionRecorder::get_instance().record(
      "set_kernel_arg_float64",
      {ActionArg("kernel_name", kernel_->name), ActionArg("arg_id", arg_id),
       ActionArg("val", d)});

  auto dt = kernel_->parameter_list[arg_id].get_dtype();
  if (dt->is_primitive(PrimitiveTypeID::f32)) {
    ctx_->set_arg(arg_id, (float32)d);
  } else if (dt->is_primitive(PrimitiveTypeID::f64)) {
    ctx_->set_arg(arg_id, (float64)d);
  } else if (dt->is_primitive(PrimitiveTypeID::i32)) {
    ctx_->set_arg(arg_id, (int32)d);
  } else if (dt->is_primitive(PrimitiveTypeID::i64)) {
    ctx_->set_arg(arg_id, (int64)d);
  } else if (dt->is_primitive(PrimitiveTypeID::i8)) {
    ctx_->set_arg(arg_id, (int8)d);
  } else if (dt->is_primitive(PrimitiveTypeID::i16)) {
    ctx_->set_arg(arg_id, (int16)d);
  } else if (dt->is_primitive(PrimitiveTypeID::u8)) {
    ctx_->set_arg(arg_id, (uint8)d);
  } else if (dt->is_primitive(PrimitiveTypeID::u16)) {
    ctx_->set_arg(arg_id, (uint16)d);
  } else if (dt->is_primitive(PrimitiveTypeID::u32)) {
    ctx_->set_arg(arg_id, (uint32)d);
  } else if (dt->is_primitive(PrimitiveTypeID::u64)) {
    ctx_->set_arg(arg_id, (uint64)d);
  } else if (dt->is_primitive(PrimitiveTypeID::f16)) {
    // use f32 to interact with python
    ctx_->set_arg(arg_id, (float32)d);
  } else {
    TI_NOT_IMPLEMENTED
  }
}

void LaunchContextBuilder::set_arg_int(int arg_id, int64 d) {
  TI_ASSERT_INFO(!kernel_->parameter_list[arg_id].is_array,
                 "Assigning scalar value to external (numpy) array argument is "
                 "not allowed.");

  ActionRecorder::get_instance().record(
      "set_kernel_arg_integer",
      {ActionArg("kernel_name", kernel_->name), ActionArg("arg_id", arg_id),
       ActionArg("val", d)});

  auto dt = kernel_->parameter_list[arg_id].get_dtype();
  if (dt->is_primitive(PrimitiveTypeID::i32)) {
    ctx_->set_arg(arg_id, (int32)d);
  } else if (dt->is_primitive(PrimitiveTypeID::i64)) {
    ctx_->set_arg(arg_id, (int64)d);
  } else if (dt->is_primitive(PrimitiveTypeID::i8)) {
    ctx_->set_arg(arg_id, (int8)d);
  } else if (dt->is_primitive(PrimitiveTypeID::i16)) {
    ctx_->set_arg(arg_id, (int16)d);
  } else if (dt->is_primitive(PrimitiveTypeID::u8)) {
    ctx_->set_arg(arg_id, (uint8)d);
  } else if (dt->is_primitive(PrimitiveTypeID::u16)) {
    ctx_->set_arg(arg_id, (uint16)d);
  } else if (dt->is_primitive(PrimitiveTypeID::u32)) {
    ctx_->set_arg(arg_id, (uint32)d);
  } else if (dt->is_primitive(PrimitiveTypeID::u64)) {
    ctx_->set_arg(arg_id, (uint64)d);
  } else {
    TI_INFO(dt->to_string());
    TI_NOT_IMPLEMENTED
  }
}

void LaunchContextBuilder::set_arg_uint(int arg_id, uint64 d) {
  set_arg_int(arg_id, d);
}

void LaunchContextBuilder::set_extra_arg_int(int i, int j, int32 d) {
  ctx_->extra_args[i][j] = d;
}

void LaunchContextBuilder::set_arg_external_array_with_shape(
    int arg_id,
    uintptr_t ptr,
    uint64 size,
    const std::vector<int64> &shape) {
  TI_ASSERT_INFO(
      kernel_->parameter_list[arg_id].is_array,
      "Assigning external (numpy) array to scalar argument is not allowed.");

  ActionRecorder::get_instance().record(
      "set_kernel_arg_ext_ptr",
      {ActionArg("kernel_name", kernel_->name), ActionArg("arg_id", arg_id),
       ActionArg("address", fmt::format("0x{:x}", ptr)),
       ActionArg("array_size_in_bytes", (int64)size)});

  TI_ASSERT_INFO(shape.size() <= taichi_max_num_indices,
                 "External array cannot have > {max_num_indices} indices");
  ctx_->set_arg_external_array(arg_id, ptr, size, shape);
}

void LaunchContextBuilder::set_arg_ndarray(int arg_id, const Ndarray &arr) {
  intptr_t ptr = arr.get_device_allocation_ptr_as_int();
  TI_ASSERT_INFO(arr.shape.size() <= taichi_max_num_indices,
                 "External array cannot have > {max_num_indices} indices");
  ctx_->set_arg_ndarray(arg_id, ptr, arr.shape);
}

void LaunchContextBuilder::set_arg_texture(int arg_id, const Texture &tex) {
  intptr_t ptr = tex.get_device_allocation_ptr_as_int();
  ctx_->set_arg_texture(arg_id, ptr);
}

void LaunchContextBuilder::set_arg_rw_texture(int arg_id, const Texture &tex) {
  intptr_t ptr = tex.get_device_allocation_ptr_as_int();
  ctx_->set_arg_rw_texture(arg_id, ptr, tex.get_size());
}

void LaunchContextBuilder::set_arg_raw(int arg_id, uint64 d) {
  TI_ASSERT_INFO(!kernel_->parameter_list[arg_id].is_array,
                 "Assigning scalar value to external (numpy) array argument is "
                 "not allowed.");

  if (!kernel_->is_evaluator) {
    ActionRecorder::get_instance().record(
        "set_arg_raw",
        {ActionArg("kernel_name", kernel_->name), ActionArg("arg_id", arg_id),
         ActionArg("val", (int64)d)});
  }
  ctx_->set_arg<uint64>(arg_id, d);
}

RuntimeContext &LaunchContextBuilder::get_context() {
  kernel_->program->prepare_runtime_context(ctx_);
  return *ctx_;
}

template <typename T>
T Kernel::fetch_ret(DataType dt, int i) {
  if (dt->is_primitive(PrimitiveTypeID::f32)) {
    return (T)program->fetch_result<float32>(i);
  } else if (dt->is_primitive(PrimitiveTypeID::f64)) {
    return (T)program->fetch_result<float64>(i);
  } else if (dt->is_primitive(PrimitiveTypeID::i32)) {
    return (T)program->fetch_result<int32>(i);
  } else if (dt->is_primitive(PrimitiveTypeID::i64)) {
    return (T)program->fetch_result<int64>(i);
  } else if (dt->is_primitive(PrimitiveTypeID::i8)) {
    return (T)program->fetch_result<int8>(i);
  } else if (dt->is_primitive(PrimitiveTypeID::i16)) {
    return (T)program->fetch_result<int16>(i);
  } else if (dt->is_primitive(PrimitiveTypeID::u8)) {
    return (T)program->fetch_result<uint8>(i);
  } else if (dt->is_primitive(PrimitiveTypeID::u16)) {
    return (T)program->fetch_result<uint16>(i);
  } else if (dt->is_primitive(PrimitiveTypeID::u32)) {
    return (T)program->fetch_result<uint32>(i);
  } else if (dt->is_primitive(PrimitiveTypeID::u64)) {
    return (T)program->fetch_result<uint64>(i);
  } else if (dt->is_primitive(PrimitiveTypeID::f16)) {
    // use f32 to interact with python
    return (T)program->fetch_result<float32>(i);
  } else {
    TI_NOT_IMPLEMENTED
  }
}

float64 Kernel::get_ret_float(int i) {
  auto dt = rets[i].dt->get_compute_type();
  return fetch_ret<float64>(dt, i);
}

int64 Kernel::get_ret_int(int i) {
  auto dt = rets[i].dt->get_compute_type();
  return fetch_ret<int64>(dt, i);
}

uint64 Kernel::get_ret_uint(int i) {
  auto dt = rets[i].dt->get_compute_type();
  return fetch_ret<uint64>(dt, i);
}

std::vector<int64> Kernel::get_ret_int_tensor(int i) {
  DataType dt = rets[i].dt->as<TensorType>()->get_element_type();
  int size = rets[i].dt->as<TensorType>()->get_num_elements();
  std::vector<int64> res;
  for (int j = 0; j < size; j++) {
    res.emplace_back(fetch_ret<int64>(dt, j));
  }
  return res;
}

std::vector<uint64> Kernel::get_ret_uint_tensor(int i) {
  DataType dt = rets[i].dt->as<TensorType>()->get_element_type();
  int size = rets[i].dt->as<TensorType>()->get_num_elements();
  std::vector<uint64> res;
  for (int j = 0; j < size; j++) {
    res.emplace_back(fetch_ret<uint64>(dt, j));
  }
  return res;
}

std::vector<float64> Kernel::get_ret_float_tensor(int i) {
  DataType dt = rets[i].dt->as<TensorType>()->get_element_type();
  int size = rets[i].dt->as<TensorType>()->get_num_elements();
  std::vector<float64> res;
  for (int j = 0; j < size; j++) {
    res.emplace_back(fetch_ret<float64>(dt, j));
  }
  return res;
}

std::string Kernel::get_name() const {
  return name;
}

void Kernel::init(Program &program,
                  const std::function<void()> &func,
                  const std::string &primal_name,
                  AutodiffMode autodiff_mode) {
  this->autodiff_mode = autodiff_mode;
  this->lowered_ = false;
  this->program = &program;

  is_accessor = false;
  is_evaluator = false;
  compiled_ = nullptr;
  context =
      std::make_unique<FrontendContext>(program.this_thread_config().arch);
  ir = context->get_root();
  ir_is_ast_ = true;

  if (autodiff_mode == AutodiffMode::kNone) {
    name = primal_name;
  } else if (autodiff_mode == AutodiffMode::kCheckAutodiffValid) {
    name = primal_name + "_validate_grad";
  } else if (autodiff_mode == AutodiffMode::kForward) {
    name = primal_name + "_forward_grad";
  } else if (autodiff_mode == AutodiffMode::kReverse) {
    name = primal_name + "_reverse_grad";
  }

  func();
  ir->as<Block>()->kernel = this;
}

// static
bool Kernel::supports_lowering(Arch arch) {
  return arch_is_cpu(arch) || (arch == Arch::cuda) || (arch == Arch::dx12) ||
         (arch == Arch::metal);
}

void Kernel::offload_to_executable(IRNode *stmt) {
  auto config = program->this_thread_config();
  bool verbose = config.print_ir;
  if ((is_accessor && !config.print_accessor_ir) ||
      (is_evaluator && !config.print_evaluator_ir))
    verbose = false;
  irpass::offload_to_executable(
      stmt, config, this, verbose,
      /*determine_ad_stack_size=*/autodiff_mode == AutodiffMode::kReverse,
      /*lower_global_access=*/true,
      /*make_thread_local=*/config.make_thread_local,
      /*make_block_local=*/
      is_extension_supported(config.arch, Extension::bls) &&
          config.make_block_local);
}

// Refactor2023:FIXME: Remove (:Temp)
void launch_kernel(Program *prog, Kernel &kernel, RuntimeContext &ctx) {
  auto fn = kernel.get_compiled_func();
  if (!fn) {
    kernel.set_compiled_func(fn = prog->compile(kernel));
  }
  TI_ASSERT(!!fn);

  fn(ctx);

  const auto config = prog->this_thread_config();
  const auto arch = config.arch;

  prog->sync = (prog->sync && arch_is_cpu(arch));
  // Note that Kernel::arch may be different from program.config.arch
  if (config.debug && (arch_is_cpu(arch) || arch == Arch::cuda)) {
    prog->check_runtime_error();
  }
}
}  // namespace taichi::lang
