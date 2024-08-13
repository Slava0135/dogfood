#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from FsState import FsState
from TestPackage import TestPackage
from Command import *

import M4

NR_SEGMENT = 3
LENGTH_PER_SEGMENT = 10

def generate_test_case(package):
    state = FsState()
    #
    # we use kmeans generation and random generation alternately.
    #
    for _ in range(NR_SEGMENT):
        state.rand_gen(LENGTH_PER_SEGMENT // 2)
        state.kmeans_gen(LENGTH_PER_SEGMENT // 2)

    #
    # Filling papameters
    #
    state.expand_param()

    test_case = package.new_test_case(state)

    test_case.save()
    M4.print_red(f'Test Case: {test_case.path_}')

    return test_case

if __name__ == "__main__":
    package = TestPackage()
    test_case = generate_test_case(package)