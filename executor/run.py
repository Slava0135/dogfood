#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import glob
import logging
import subprocess
import os
import stat


log = logging.getLogger()
log.setLevel(logging.DEBUG)

formatter = logging.Formatter('%(asctime)s [%(levelname)s] %(message)s')

ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)
ch.setFormatter(formatter)

fh = logging.FileHandler("run.log")
fh.setLevel(logging.DEBUG)
fh.setFormatter(formatter)

log.addHandler(ch)
log.addHandler(fh)


issues_found = 0


class TestCase:

    def __init__(self, name: str, exe: str) -> None:
        self.name = name
        self.exe = exe
        self.outputs = dict()
        self.traces = dict()
    
    def compare_and_clear(self):
        global issues_found
        if len(set(self.outputs.values())) > 1:
            log.info(f"different test outputs found!")
            issues_found += 1
            for fs, output in self.outputs.items():
                with open(f"{self.name}.{fs}.output", "w") as file:
                    file.write(output)
        if len(set(self.traces.values())) > 1:
            log.info(f"different test traces found!")
            issues_found += 1
            for fs, output in self.traces.items():
                with open(f"{self.name}.{fs}.trace", "w") as file:
                    file.write(output)
        self.outputs.clear()
        self.traces.clear()

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
            self.__workspace = result.stdout.replace("\n", "")

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

    def run(self, tc: TestCase):
        result = subprocess.run(
            [f"./{tc.exe}", f"{self.__workspace}/root"],
            capture_output = True,
            text = True
        )
        tc.outputs[self.name] = result.stdout + result.stderr
        try:
            with open('trace.dat') as f:
                tc.traces[self.name] = f.read()
        except:
            log.warning(f"trace not found")
            tc.traces[self.name] = ""


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
    for exe in glob.glob("*.out"):
        name = exe.removesuffix(".out")
        source = glob.glob(f"{name}.c")
        if not source:
            raise TestCaseSourceNotFoundError(f"source for testcase '{name}' not found")
        mark_executable(exe)
        testcases.append(TestCase(name, exe))
    return testcases

def run_testcases(systems: list[FileSystemUnderTest], testcases: list[TestCase]):
    for tc in testcases:
        log.info(f"running testcase '{tc.name}'...")
        for fs in systems:
            log.info(f"testing {fs.name}...")
            fs.setup()
            fs.run(tc)
            fs.teardown()
        log.info(f"comparing results...")
        tc.compare_and_clear()


if __name__ == "__main__":
    log.info("initializing runner...")
    try:
        systems = lookup_systems()
        testcases = lookup_testcases()
    except Exception as e:
        log.error(e)
    else:
        log.info(f"found {len(systems)} filesystems under test: {[s.name for s in systems]}")
        log.info(f"found {len(testcases)} testcases")
        log.info(f"running testcases...")
        try:
            run_testcases(systems, testcases)
        except Exception as e:
            log.error(f"critical error when running testcases, aborting...\n{e}")
        else:
            log.info(f"done!")
        log.info(f"found {issues_found} issues")
