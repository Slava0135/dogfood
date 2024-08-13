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

EXECUTOR_PATH = "../executor/"

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
    old_dir = os.getcwd()
    build_files = glob.glob(EXECUTOR_PATH + "*.h") + glob.glob(EXECUTOR_PATH + "*.cc") + glob.glob(EXECUTOR_PATH + "makefile")
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
    M4.print_cyan(f'Removing build files...')
    for f in glob.glob("*.h") + glob.glob("*.cc") + glob.glob("makefile") + glob.glob("*.o"):
        os.remove(f)
    M4.print_green(f'BUILD SUCCESS')
    os.chdir(old_dir)

def add_runner(path):
    M4.print_blue(f'Copying runner to {path}')
    shutil.copy(EXECUTOR_PATH + "run.py", path)

if __name__ == "__main__":
    config.use_config('Seq')
    package = TestPackage()
    testcases = []
    for _ in range(config.get('NR_TESTCASE_PER_PACKAGE')):
        testcases.append(generate_test_case(package))
    build_testcases(package.package_path_, testcases)
    add_runner(package.package_path_)