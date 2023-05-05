#include "exports.h"

#include <cstdio>
#include <cstring>

#include "taichi/program/program.h"

int ticore_hello_world(const char *extra_msg) {
  std::printf("Hello World! %s\n", extra_msg);
  return std::strlen(extra_msg);
}

int ticore_compile_and_launch_kernel(void *program,
                                     const void *kernel,
                                     void *launch_ctx) {
  using namespace taichi::lang;

  // Temporary

  if (program == nullptr || kernel == nullptr || launch_ctx == nullptr) {
    TI_WARN("Null pointer detected in ticore_compile_and_launch_kernel");
    return -1;
  }

  try {
    auto &t_program = *reinterpret_cast<Program *>(program);
    const auto &t_kernel = *reinterpret_cast<const Kernel *>(kernel);
    auto &t_luanch_ctx = *reinterpret_cast<LaunchContextBuilder *>(launch_ctx);

    const auto &ckd = t_program.compile_kernel(
        t_program.compile_config(), t_program.get_device_caps(), t_kernel);
    t_program.launch_kernel(ckd, t_luanch_ctx);
    return 0;
  } catch (const std::exception &e) {
    TI_WARN("Exception caught: {}", e.what());
    return -1;
  }
}
