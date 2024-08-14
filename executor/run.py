#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import glob
import logging
import subprocess
import os
import stat

log = logging.getLogger()
log.setLevel(logging.DEBUG)
ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)
formatter = logging.Formatter('%(asctime)s [%(levelname)s] %(message)s')
ch.setFormatter(formatter)
log.addHandler(ch)

class FileSystemUnderTest:

    def __init__(self, name: str, setup: str, teardown: str) -> None:
        self.name = name
        self.__setup = setup
        self.__teardown = teardown
        self.__workspace = None

    def setup(self):
        if self.__workspace != None:
            raise AlreadySetupError(f"filesystem {self.name} was already setup")
        try:
            result = subprocess.run(
                [f"./{self.__setup}"],
                capture_output = True,
                text = True
            )
        except Exception as e:
            raise SetupError(f"failed to setup filesystem {self.name}") from e
        else:
            if result.returncode != 0:
                raise SetupError(f"failed to setup filesystem {self.name}:\n{result.stderr}")
            self.__workspace = result.stdout

    def teardown(self):
        if self.__workspace == None:
            raise WasNotSetupError(f"filesystem {self.name} was not setup when calling teardown")

        try:
            result = subprocess.run(
                [f"./{self.__teardown}", self.__workspace],
                capture_output = True,
                text = True
            )
        except Exception as e:
            raise TeardownError(f"failed to teardown filesystem {self.name}") from e
        else:    
            if result.returncode != 0:
                raise TeardownError(f"failed to teardown filesystem {self.name}:\n{result.stderr}")
            self.__workspace = None

class TestCase:
    
    def __init__(self, name: str) -> None:
        self.name = name
        self.traces = dict()

class TeardownScriptNotFoundError(Exception):
    pass
class AlreadySetupError(Exception):
    pass
class WasNotSetupError(Exception):
    pass
class SetupError(Exception):
    pass
class TeardownError(Exception):
    pass

class TestCaseSourceNotFoundError(Exception):
    pass

def mark_executable(path: str):
    os.chmod(path, os.stat(path).st_mode | stat.S_IEXEC)

def lookup_systems() -> list[FileSystemUnderTest]:
    systems = []
    for setup in glob.glob("setup-*"):
        name = setup.removeprefix("setup-")
        teardown_scripts = glob.glob(f"teardown-{name}")
        if not teardown_scripts:
            raise TeardownScriptNotFoundError(f"teardown script for filesystem '{name}' not found")
        teardown = teardown_scripts.pop()
        mark_executable(setup)
        mark_executable(teardown)
        systems.append(FileSystemUnderTest(name, setup, teardown))
    return systems

def lookup_testcases() -> list[TestCase]:
    testcases = []
    for tc in glob.glob("*.out"):
        name = tc.removesuffix(".out")
        source = glob.glob(f"{name}.c")
        if not source:
            raise TestCaseSourceNotFoundError(f"source for testcase '{name}' not found")
        mark_executable(tc)
        testcases.append(TestCase(name))
    return testcases

if __name__ == "__main__":
    try:
        systems = lookup_systems()
        testcases = lookup_testcases()
    except Exception as e:
        log.error(e)
    else:
        log.info(f"found {len(systems)} filesystems under test: {[s.name for s in systems]}")
        log.info(f"found {len(testcases)} testcases")
