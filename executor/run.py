#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import glob
import logging
import subprocess
import os
import stat
import functools
import argparse


log = logging.getLogger()
log.setLevel(logging.DEBUG)

formatter = logging.Formatter('%(asctime)s [%(levelname)s] %(message)s')
formatter.datefmt = "%Y-%m-%d %H:%M:%S"

ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)
ch.setFormatter(formatter)

fh = logging.FileHandler("run.log")
fh.setLevel(logging.DEBUG)
fh.setFormatter(formatter)

log.addHandler(ch)
log.addHandler(fh)


issues_found = 0
asfs = None

class TestCase:

    def __init__(self, name: str, exe: str) -> None:
        self.name = name
        self.exe = exe
        self.outputs = dict()
        self.traces = dict()
        self.abs_states = dict()
    
    def compare_and_clear(self):
        global issues_found
        if len(set([len(x.splitlines()) for x in self.outputs.values()])) > 1:
            log.warning(f"different test outputs found!")
            issues_found += 1
            groups = dict()
            for fs, output in self.outputs.items():
                groups[output] = groups.get(output, []) + [fs]
                with open(f"{self.name}.{fs}.output", "w") as file:
                    file.write(output)
            log.warning(f"equivalent outputs: {list(groups.values())}")
        if len(set(self.traces.values())) > 1:
            log.warning(f"different test traces found!")
            issues_found += 1
            groups = dict()
            for fs, output in self.traces.items():
                groups[output] = groups.get(output, []) + [fs]
                with open(f"{self.name}.{fs}.trace.csv", "w") as file:
                    file.write(output)
            log.warning(f"equivalent traces: {list(groups.values())}")
        if len(set(self.abs_states.values())) > 1:
            log.warning(f"different abstract states found!")
        self.outputs.clear()
        self.traces.clear()
        self.abs_states.clear()

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
            [f"./{tc.exe}", f"{self.__workspace}"],
            capture_output = True,
            text = True
        )
        tc.outputs[self.name] = result.stdout + result.stderr
        try:
            with open('trace.csv') as f:
                tc.traces[self.name] = f.read()
        except:
            log.warning(f"trace not found")
            tc.traces[self.name] = ""
        if asfs:
            result = subprocess.run(
                [f"./{asfs}", f"{self.__workspace}", "-lm"],
                capture_output = True,
                text = True
            )
            if result.returncode:
                log.warning(f"failed to eval abstract state for filesystem {self.name}:\n{result.stderr}")
                tc.abs_states[self.name] = ""
            else:
                tc.abs_states[self.name] = result.stdout


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

def init_asfs():
    global asfs
    f = glob.glob("asfs")
    if f:
        asfs = f.pop()
    else:
        log.warning("'abstract state filesystem' tool was not found!")

def run_testcases(systems: list[FileSystemUnderTest], testcases: list[TestCase], first: int | None, last: int | None):
    def compare(a: TestCase, b: TestCase):
        a = a.name
        b = b.name
        if a.isnumeric() and b.isnumeric():
            return int(a) - int(b)
        elif a.isnumeric():
            return 1
        elif b.isnumeric():
            return -1
        else:
            if a > b:
                return 1
            elif a < b:
                return -1
            else:
                return 0
    index = 0
    testcases = [tc for tc in sorted(testcases, key=functools.cmp_to_key(compare))
                 if (not tc.name.isnumeric()) or (not first or int(tc.name) >= first) and (not last or int(tc.name) <= last)]
    for tc in sorted(testcases, key=functools.cmp_to_key(compare)):
        index += 1
        log.info(f"running testcase '{tc.name}' ({index}/{len(testcases)})")
        for fs in systems:
            log.info(f"testing {fs.name}...")
            fs.setup()
            try:
                fs.run(tc)
            except:
                fs.teardown()
                raise
            fs.teardown()
        log.info(f"comparing results...")
        tc.compare_and_clear()


if __name__ == "__main__":

    parser = argparse.ArgumentParser("Run testcases.")
    parser.add_argument('--from', help="First testcase to run. (inclusive)", type=int)
    parser.add_argument('--to', help="Last testcase to run. (inclusive)", type=int)
    args = parser.parse_args()

    log.info("initializing runner...")
    try:
        systems = lookup_systems()
        testcases = lookup_testcases()
        init_asfs()
    except Exception as e:
        log.critical("when initializing runner", exc_info=e)
    else:
        log.info(f"found {len(systems)} filesystems under test: {[s.name for s in systems]}")
        log.info(f"found {len(testcases)} testcases")
        log.info(f"running testcases...")
        try:
            run_testcases(systems, testcases, getattr(args, "from", None), getattr(args, "to", None))
        except Exception as e:
            log.critical(f"when running testcases", exc_info=e)
        else:
            log.info(f"DONE!")
        log.info(f"found {issues_found} issues")
