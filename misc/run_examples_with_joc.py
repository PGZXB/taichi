# Run examples with `offline_cache=True`

# Workflow: Get all examples to run -> For each e in examples:
#               Get src code of e -> Insert get-scoped_prof-info code into e as c:
#                   Run c(force exit if time exceed) with `offline_cache=False` -> Get prof_info as p:
#                       Parse p -> Get time spent on compilation as a
#                   Run c(force exit if time exceed) with `offline_cache=True` -> Get prof_info as p:
#                       Parse p -> Get time spent on compilation as b
#                   Run c(force exit if time exceed) with `offline_cache=True` -> Get prof_info as p:
#                       Parse p -> Get time spent on compilation as c
#               Save a, b, c in data
#           Generate csv and chars by data

# The result's format:
#   CSV:
#       name, time_no_cache, time_with_cache_1st, time_with_cache_2nd
#   Chars:
#       (histogram)

import subprocess
import glob
import os
import sys
from tempfile import mkdtemp, mktemp

ext_code = """
import taichi as ti
import functools
import io
import sys
_kernel_dec = ti.kernel
_rewjoc_cnt = 0
def _rewjoc_dec(func):
    global _rewjoc_cnt
    _rewjoc_cnt += 1
    func._1st_run = True
    @functools.wraps(func)
    def wrapped(*args, **kwargs):
        ret = _kernel_dec(func)(*args, **kwargs)
        if func._1st_run:
            global _rewjoc_cnt
            _rewjoc_cnt -= 1
            func._1st_run = False
        print('CNT =', _rewjoc_cnt)
        if _rewjoc_cnt == 0:
            saved_stdout = sys.stdout
            oss = io.StringIO()
            sys.stdout = oss
            ti.profiler.print_scoped_profiler_info()
            sys.stdout = saved_stdout
            content: str = oss.getvalue()
            lines = content.splitlines()
            for l in lines:
                if 'compile' in l:
                    l = l[0: l.find('compile')]
                    l = l.strip()
                    t, u = l.split(' ')
                    tu = u.strip()
                    helper = {'s': 1e3, 'ms': 1e1, 'us': 1e-3, 'ns': 1e-6, 'm': 60 * 1e3, 'h': 60 * 60 * 1e3}
                    t *= helper[tu]
                    with open('./run-examples-with-JOC-data.txt', 'a') as f:
                        f.write(f'{__file__} {t}ms\\n')
                    break
            exit(0)
        return ret
    return wrapped
ti.kernel = _rewjoc_dec
"""

def process_example_code(lines: str, offline_cache: bool):
    # (Bad Practice)
    for l in lines:
        if 'ti.init' in l:
            l = l[: l.find('#')]
            l = l.strip()
            assert l[-1] == ')'
            l = l[0:len(l) - 1] + ','
            l += f'offline_cache={offline_cache})'
    result = [ext_code]
    result.extend(lines)
    return result


def main(examples_path):
    tmpfile = mktemp()
    files = glob.glob(f'{examples_path}/**/*.py', recursive=True)
    for name in files:
        print(f'Running {name}')
        with open(name) as f:
            lines = process_example_code(f.readlines(), False)
            with open(tmpfile, 'w') as tf:
                tf.writelines(lines)
            try:
                subprocess.call(f'python3 {tmpfile}')
            except subprocess.TimeoutExpired:
                pass
            # lines = process_example_code(f.readlines(), True)
            # lines = process_example_code(f.readlines(), True)
            


if __name__ == '__main__':
    tmpdir = mkdtemp()
    os.environ['TI_OFFLINE_CACHE_FILE_PATH'] = tmpdir
    main(sys.argv[1])