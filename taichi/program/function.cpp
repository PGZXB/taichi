#include "taichi/program/function.h"
#include "taichi/program/program.h"
#include "taichi/ir/transforms.h"
#include "taichi/analysis/offline_cache_util.h"

namespace taichi::lang {

Function::Function(Program *program, const FunctionKey &func_key)
    : func_key(func_key) {
  this->program = program;
  arch = program->compile_config().arch;
}

void Function::set_function_body(const std::function<void()> &func) {
  context = std::make_unique<FrontendContext>(program->compile_config().arch);
  ir = context->get_root();
  ir_type_ = IRType::AST;

  func();
  finalize_params();
  finalize_rets();
}

void Function::set_function_body(std::unique_ptr<IRNode> func_body,
                                 IRType ir_type) {
  ir = std::move(func_body);
  ir_type_ = ir_type;
}

std::string Function::get_name() const {
  return func_key.get_full_name();
}

}  // namespace taichi::lang
