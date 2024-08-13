#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from FsState import FsState
from TestPackage import TestPackage
from Command import *

import M4
import config
import glob
import shutil
import subprocess
import os

def generate_test_case(package):
    state = FsState()
    #
    # we use kmeans generation and random generation alternately.
    #
    for _ in range(config.get('NR_SEGMENT')):
        state.rand_gen(config.get('LENGTH_PER_SEGMENT') // 2)
        state.kmeans_gen(config.get('LENGTH_PER_SEGMENT') // 2)

    #
    # Filling papameters
    #
    state.expand_param()

    test_case = package.new_test_case(state)

    test_case.save()
    M4.print_red(f'Test Case: {test_case.path_}')

    return test_case

def build_testcases(path, testcases):
    base_path = "../executor/"
    build_files = glob.glob(base_path + "*.h") + glob.glob(base_path + "*.cc") + glob.glob(base_path + "makefile")
    M4.print_green(f'Copying build files to {path}: {build_files}')
    for f in build_files:
        shutil.copy(f, path)
    os.chdir(path)
    for tc in testcases:
        M4.print_blue(f'Building {tc.path_}...')
        filename_with_ext = os.path.basename(tc.path_)
        filename = os.path.splitext(filename_with_ext)[0]
        ret = subprocess.call(['make', f'TESTNAME={filename}'])
        if ret != 0:
            M4.print_red(f'BUILD ERROR')
            return
    M4.print_green(f'BUILD SUCCESS')

if __name__ == "__main__":
    config.use_config('Seq')
    package = TestPackage()
    testcases = []
    for _ in range(config.get('NR_TESTCASE_PER_PACKAGE')):
        testcases.append(generate_test_case(package))
    build_testcases(package.package_path_, testcases)
