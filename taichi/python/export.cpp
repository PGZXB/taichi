/*******************************************************************************
    Copyright (c) The Taichi Authors (2016- ). All Rights Reserved.
    The use of this software is governed by the LICENSE file.
*******************************************************************************/

#include "taichi/python/export.h"
#include "taichi/common/interface.h"
#include "taichi/util/io.h"

namespace taichi {

namespace py { GTiCoreStat g_ticore_stat; };

PYBIND11_MODULE(taichi_python, m) {
  m.doc() = "taichi_python";

  for (auto &kv : InterfaceHolder::get_instance()->methods) {
    kv.second(&m);
  }

  py::module wpm{&m};

  export_lang(wpm);
  export_math(wpm);
  export_misc(wpm);
  export_visual(wpm);
  export_ggui(wpm);
}

}  // namespace taichi
