#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import glob
import logging

log = logging.getLogger('simple_example')
log.setLevel(logging.DEBUG)
ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)
formatter = logging.Formatter('%(asctime)s [%(levelname)s] %(message)s')
ch.setFormatter(formatter)
log.addHandler(ch)

class FileSystemUnderTest:

    def __init__(self, name: str, setup: str, teardown: str) -> None:
        self.name = name
        self.setup = setup
        self.teardown = teardown

    def setup():
        pass

    def teardown():
        pass

class TeardownScriptNotFoundError(Exception):
    pass

def lookup_systems() -> list[FileSystemUnderTest]:
    systems = []
    for setup in glob.glob("setup-*"):
        name = setup.removeprefix("setup-")
        teardown_scripts = glob.glob(f"teardown-{name}")
        if len(teardown_scripts) == 0:
            raise TeardownScriptNotFoundError(f"teardown script for filesystem '{name}' not found")
        teardown = teardown_scripts.pop()
        systems.append(FileSystemUnderTest(name, setup, teardown))
    return systems

if __name__ == "__main__":
    try:
        systems = lookup_systems()
    except TeardownScriptNotFoundError as e:
        log.error(e)
    else:
        system_names = [s.name for s in systems]
        system_n = len(systems)
        log.info(f"Found {system_n} filesystems under test: {system_names}")
