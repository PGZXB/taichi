#pragma once

#include <memory>
#include <filesystem>  // NOTE:WILL_DELETE: Will be deleted
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

#include "llvm/AsmParser/Parser.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_os_ostream.h"

#include "spdlog/fmt/bundled/core.h"
#include "taichi/common/core.h"
#include "taichi/common/logging.h"
#include "taichi/ir/ir.h"
#include "taichi/program/compile_config.h"

/* TODO:WILL_DELETE: llvm后端offline cache文件格式

  暂时采用离散的易读的文件格式(多个文件)
  需要存储的文件 :
    CompileConfig --> config.txt
    每个Kernel(grad不grad视为两个不同的kernel)一组文件(临时的, 实际还要考虑free
  vars, template具体参数类型) 时间戳+src-code --> {kernel_name}_time_code.txt
  (格式(binary打开): {时间戳}\n{src-code} llvm::Module --> {kernel_name}.ll
  (这里的kernel_name格式待定不能用以前那种对于相同的kernel每次运行有可能的不同的名字
      offloaded_task_name_list --> {kernel_name}_otnl.txt (格式(binary打开):
  {name1}\n{name2}\n...

*/

TLANG_NAMESPACE_BEGIN
struct LlvmOfflineCache {
  struct KernelCache {
    std::string kernel_name;
    uint64 last_mtime{
        0};  // NOTE:WILL_DELETE: 用double不好, 但由于Python获取lmt返回的是浮点,
             // 用C++获取要用系统API(不知Taichi有没有封装, 待定
    std::string src_code;
    std::unique_ptr<llvm::Module> owned_module;
    llvm::Module *module{nullptr};
    std::vector<std::string> offloaded_task_name_list;

    KernelCache() = default;
    KernelCache(KernelCache &&) = default;
    KernelCache &operator=(KernelCache &&) = default;
    ~KernelCache() = default;
  };

  CompileConfig compile_config{};
  std::unordered_map<std::string, KernelCache> kernels;
};

class LlvmOfflineCacheFileReader {
 public:
  LlvmOfflineCacheFileReader(const std::string &path) : path_(path) {
    namespace fs = std::filesystem;
    fmt::print("TEMP$$$ Create LlvmOfflineCacheFileReader on {}\n",
               fs::absolute(fs::path(path)).string());
  }

  CompileConfig get_compile_config() {
    TI_NOT_IMPLEMENTED
    return {};
  }

  LlvmOfflineCache::KernelCache get_kernel_cache(const std::string &kernel_name,
                                                 llvm::LLVMContext &llvm_ctx) {
    LlvmOfflineCache::KernelCache res;
    res.kernel_name = kernel_name;
    std::string filename_prefix = path_ + "/" + kernel_name;
    {
      std::string filename = filename_prefix + "_time_code.txt";
      std::ifstream in(filename, std::ios::in | std::ios::binary);
      if (!in.is_open())
        return {};
      in >> res.last_mtime;
      TI_ASSERT(in.peek() == '\n');
      in.get();
      std::ostringstream oss;
      oss << in.rdbuf();
      res.src_code = oss.str();
    }
    {
      std::string filename = filename_prefix + ".ll";
      llvm::SMDiagnostic err;
      res.owned_module = llvm::parseAssemblyFile(filename, err, llvm_ctx);
      res.module = res.owned_module.get();
      if (!res.module)
        return {};
    }
    {
      std::string filename = filename_prefix + "_otnl.txt";
      std::ifstream in(filename, std::ios::in | std::ios::binary);
      if (!in.is_open())
        return {};
      while (true) {  // NOTE:WILL_DELETE: 不允许空行,
                      // 整理到文档里(未来会搞二进制格式, 问题不大
        std::string line;
        std::getline(in, line, '\n');
        if (line.empty())
          break;
        res.offloaded_task_name_list.push_back(std::move(line));
      }
    }
    return res;
  }

 public:
  std::string path_;
};

class LlvmOfflineCacheFileWriter {
 public:
  LlvmOfflineCacheFileWriter(const std::string &path) : path_(path) {
    namespace fs = std::filesystem;
    fmt::print("TEMP$$$ Create LlvmOfflineCacheFileWriter on {}\n",
               fs::absolute(fs::path(path)).string());
  }

  void set_data(LlvmOfflineCache &&data) {
    this->data_ = std::move(data);
  }

  void set_compile_config(const CompileConfig &config) {
    this->data_.compile_config = config;
  }

  void add_kernel_cache(const std::string &kernel_name,
                        LlvmOfflineCache::KernelCache &&kernel_cache) {
    data_.kernels[kernel_name] =
        std::move(kernel_cache);  // FIXME:WILL_DELETE: 是否要检测重复添加?
  }

  void dump() {
    // FIXME:WILL_DELETE: 实现dump CompileConfig

    for (const auto &[k, v] : data_.kernels) {
      std::string filename_prefix = path_ + "/" + k;
      {
        std::string filename = filename_prefix + "_time_code.txt";
        std::ofstream os(filename, std::ios::out | std::ios::binary);
        TI_ERROR_IF(!os.is_open(), "File {} open failed", filename);
        os << v.last_mtime << '\n' << v.src_code;
      }
      {
        std::string filename = filename_prefix + ".ll";
        std::ofstream os(filename, std::ios::out | std::ios::binary);
        TI_ERROR_IF(!os.is_open(), "File {} open failed", filename);
        llvm::SMDiagnostic err;
        llvm::LLVMContext ctx;
        llvm::raw_os_ostream llvm_os(os);
        if (v.module)
          v.module->print(llvm_os, nullptr);
        else if (v.owned_module)
          v.owned_module->print(llvm_os, nullptr);
        else
          TI_ASSERT(false);
      }
      {
        std::string filename = filename_prefix + "_otnl.txt";
        std::ofstream os(filename, std::ios::out | std::ios::binary);
        TI_ERROR_IF(!os.is_open(), "File {} open failed", filename);
        for (const auto &name : v.offloaded_task_name_list) {
          os << name << '\n';
        }
      }
    }
  }

 private:
  std::string path_;
  LlvmOfflineCache data_;
};

TLANG_NAMESPACE_END
