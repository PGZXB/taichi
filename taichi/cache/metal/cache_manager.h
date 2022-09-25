#pragma once

#include "taichi/program/compile_config.h"
#include "taichi/runtime/metal/kernel_utils.h"
#include "taichi/util/offline_cache.h"

namespace taichi::lang {
namespace metal {

class CacheManager {
  using CompiledKernelData = taichi::lang::metal::CompiledKernelData;
 public:
  using Metadata = offline_cache::Metadata;
  enum Mode { NotCache, MemCache, MemAndDiskCache };

  struct Params {
     // implementation defined
  };

  CacheManager(Params &&init_params);
  
  // Load from memory || Load from disk || (Compile && Cache the result in memory)
  CompiledKernelData load_or_compile(const CompileConfig *config, const DeviceCaps *caps, Kernel *kernel);

  // Dump the cached data in memory to disk
  void dump_with_merging() const;

  // Run offline cache cleaning
  void clean_offline_cache(offline_cache::CleanCachePolicy policy,
                           int max_bytes,
                           double cleaning_factor) const;

 private:
  /* implementation defined */;
};

}  // namespace metal
}  // namespace taichi::lang
