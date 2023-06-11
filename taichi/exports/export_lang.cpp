#include "exports.h"

#include <cstdio>
#include <cstring>

#include "taichi/program/kernel.h"

// NOTE: Temporary solution, to test. No error full-checking. Not best practice.

int ticore_hello_world(const char *extra_msg) {
  std::printf("Hello World! %s\n", extra_msg);
  return std::strlen(extra_msg);
}

int ticore_make_launch_context_EMPTY(const void *p_kernel, void **ret_ctx) {
  return 0;
}

int ticore_make_launch_context(const void *p_kernel, void **ret_ctx) {
  if (!ret_ctx) {
    return -1;
  }
  auto *kernel = (taichi::lang::Kernel *)p_kernel;
  auto *ctx = kernel->make_launch_context_pp();  // NOTE: Not Safe
  *ret_ctx = ctx;
  return 0;
}

int ticore_launch_context_set_arg_int(void *p_ctx,
                                      int p_arg_id,
                                      int64_t p_val) {
  if (!p_ctx) {
    return -1;
  }
  auto *ctx = (taichi::lang::LaunchContextBuilder *)p_ctx;
  ctx->set_arg_int(p_arg_id, p_val);
  return 0;
}

int ticore_launch_context_set_arg_int_EMPTY(void *p_ctx,
                                            int p_arg_id,
                                            int64_t p_val) {
  return 0;
}