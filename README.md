# Dogfood for Differential Testing

Dogfood is a prototype tool to generate workloads and test filesystems.

This fork aims to run sequential scenario for Dogfood without QEMU (see original description below) and use it for "Differential Testing".

C++ files in `/executor` were taken from provided Docker image but were heavily refactored.

## Install

Create Python virtual environment and install dependencies from `requirements.txt`

Install GCC and Make.

Tested on Python 3.12 and GCC 14.2

## Usage

```sh
cd py-code
./GenSeq
```

This will generate and compile source files in `workspace/C-output/<TESTDIR>`

`*.c` files are generated source for a test, linked with executor from `/executor`

`*.out` are compiled binaries. They can be run separately: use `./<TESTNAME>.out <WORKSPACE_DIR>` to execute syscalls in WORKSPACE_DIR. Directory will be created if not existed.

`run.py` is runner that executes all tests from the TESTDIR. Use `./run.py`. This will write logs into console during testing. Logs are also written into `run.log`.

For each filesystem test trace is generated in CSV format. Traces and outputs are compared after each test and are saved if a difference in behavior (result code / errno) was found, see "Differential Testing".

Compare using diff: `diff <TESTNAME>.ext4.trace.csv <TESTNAME>.btrfs.trace.csv`

Optionally, you can provide an executable file named `asfs` to testcase directory, to compare filesystem states (via hashes).
[Implementation TBD]

## Adding new filesystem

In `/wrappers` there are scripts for setting each filesystem. Filesystems are detected dynamically when running `run.py`. Scripts can be written in Bash, Python, Perl or anything really.

Each filesystem must have `setup-<FSNAME>` script that must print into stdout directory with mounted filesystem (and nothing else!) after it was set up.

Each filesystem must have `teardown-<FSNAME>` script that accepts string/path of the directory with previously mounted filesystem, tearing it down so setup script can be run again.

Use existing scripts as reference.

## Configuration

In `config.py` edit `SeqConfig` class.

Most notable:

`NR_SEGMENT * LENGTH_PER_SEGMENT` = number of syscalls in every generated test. More info in original work.

`NR_TESTCASE_PER_PACKAGE` = number of tests generated.

You can also configure probability of each command (or disable altogether by setting to zero). Some of them are not working in current reimplementation.

## Results

This project is WIP and no bugs/disruptances were found yet.

If you aim for finding Linux kernel crashes then better use original work, because the runner will crash too.

Some false positives are expected, because more than 1 correct behavior can exist. So far this happens with `read`/`write` calls when multiple file descriptors are open.

## Copyright

Original work by Dongjie Chen (midwinter1993)

---

[![DOI](https://zenodo.org/badge/234754374.svg)](https://zenodo.org/badge/latestdoi/234754374)

# Artifact evaluation for Dogfood

## Overview

Dogfood is a prototype tool to generate workloads and test file systems.
In this artifact,
we introduce its basic usage,
how to use it to reproduce the experiment results in the paper,
and how to reuse it for further file system testing.

## Obtain the artifact

We have upload a docker image containing all necessary files and subjects.
[HERE](https://hub.docker.com/r/midwinter/dogfood)

Dogfood should be used under a Linux operating system.
Ubuntu-16.04 and Ubuntu-18.04 are recommendations as we have tested Dogfood under them.
*Note* that a virtual machine such as virtual-box or vmware may not work because Dogfood leverages the KVM,
which may be unavailable in a virtual environment (see `INSTALL.md`).

Please follow the instructions in `INSTALL.md` to complete the installation.

## Reproduce results in the paper

Since there are three testing scenarios are presented in the paper,
we introduce steps to reproduce the results, respectively.

### The sequential scenario

#### Reproduce the detected bugs

The tool `tmux` is preferred to open multiple terminals:

```bash
tmux -2 # Start tmux
Ctrl+b c # Start another terminal window
Ctrl+b w # Switch between the windows; learn more from the tmux documentations
```

As shown in the `INSTALL.md`, start QEMU first:

```bash
$qemu: ./ctrl start
```

Create a virtual machine snapshot:

```bash
$test: ./ctrl init
```

Run bug case scripts:

```bash
$test: ./bugcase/<subdirectory>/trigger.sh
```

The status of each bug is listed in the file `workspace/bugcase/buglist.md`

#### Compare with Syzkaller for long-time running

Run dogfood is easy:

```bash
$qemu: ./ctrl start # Start QEMU
$test: ./ctrl init
$test: ./food --fs=<file system> --test
```

After running for 12 hours,
use the `workspace/analyze-log.py` to extract the number of crashes.

The `<file system>` should be one of
(ext4, btrfs, f2fs, reiserfs, jfs, gfs2, ocfs2).

Run Syzkaller need some extra steps:

1. Install Syzkaller, follow the [instructions](https://github.com/google/syzkaller/blob/master/docs/contributing.md#go)
2. Create a disk formatted by a file system
(we provide scripts to make it easy, run):

```bash
cd workspace/objs/debian-image
./create-image.sh
./create-fs-image.sh <file system>
```

Which means:

- Create a Debian Linux Image, [instructions](https://github.com/google/syzkaller/blob/master/docs/linux/setup_ubuntu-host_qemu-vm_x86-64-kernel.md#image)
- Create a disk formatted by a file system containing the Debian Linux tools

3. Run Syzkaller

We have provided a Syzkaller configuration and a script.

- Specify `workspace/syzkaller/my.cfg` file with two parameters: `$WORKSPACE_DIR`---the absolute path of `workspace/`, and the absolute path where Syzkaller installed.

If Syzkaller is installed according to the above-mentioned instructions successfully, just run:

```bash
./run.sh
```

After running for 12 hours, check the results.

### The crash consistency scenario

[Crashmonkey](https://github.com/utsaslab/crashmonkey) works on specific kernels.
To ease the burden of [setting](https://github.com/utsaslab/crashmonkey#setup),
we provide a VM image, [download here](https://drive.google.com/file/d/1qxIqszkJd-WWtQfHB4taWvLWbM7reCy7/view?usp=sharing),
which is based on Ubuntu-16.04 but with the specific kernel and containing crashmonkey.

```bash
tar -xzvf ubuntu-16.04-clone.qcow2.tar.gz
sudo apt-get install virt-manager
sudo virt-manager
```

Import the VM image (in virt-manager, click):

> File -> New Virtual Machine -> Importing existing disk image -> Forward -> Browse -> Browse Local -> Choose the path of qcow2 file, Choose OS type: Linux, Version: Ubuntu 16.04 -> Forward -> Forward -> Finish

Start up the VM and login via ssh:

```bash
ssh icse20@192.168.122.106
# password: icse
```

If the IP address of the VM is not sure, use the following commands to check it:

```bash
sudo virsh net-list
```

That should display like this:

```
Name     State   Autostart Persistent
---------------------------------------
default  active  yes       yes
```

Then, use the following command to check the IP address.

```bash
sudo virsh net-dhcp-leases default # `default` is the name from the last result
```

Now, we work in the VM under directory `/home/icse20`.

All workloads generate by ACE is located at directory `/home/icse20/crashmonkey/code/tests/generated_workloads`.

All workloads generated by Dogfood is located at directory `/home/icse20/subjects/cmp-cases` and `/home/icse20/subjects/large-cases`.
The former is used to compared with ACE and the latter consists of a great number of workloads (presented in the paper).

Run workloads by ACE:

```bash
cd crashmonkey

sudo python xfsMonkey.py -f /dev/vda -d /dev/cow_ram0 -t <fs> -e 102400 -u build/tests/generated_workloads/ > outfile
```
`<fs>` should be one of (f2fs, ext4, btrfs).

Run workloads by Dogfood:

```bash
cp subjects/cmp-cases/yy-*.cpp dogfood/ # comparison cases
# or
cp subjects/large-cases/zz-*.cpp dogfood/ # large cases

cd crashmonkey
make

sudo python xfsMonkey.py -f /dev/vda -d /dev/cow_ram0 -t <fs> -e 102400 -u build/tests/dogfood/ > outfile
```

Run the command to count #detected issues
```bash
./diff_count.py
```

Note that before each run, clean redundant files by

```bash
./clean.sh
```

You can also use script `dogfood/py-code/b3Food.py` to generate more subjects (generated in direcotry `output/`) and copy these subjects to the directory `dogfood` in the VM.

### The concurrency scenario

We have generate 500 workloads for concurrency testing,
which is located at `workspace/con-subjects` after being extracted.

```bash
tar -xzvf con-subjects.tar.gz
```

Configure `FS` in `run-con-subjects.sh` and execute it:

```bash
./run-con-subjects
```

## Reuse Dogfood: automatic testing

Reuse dogfood to test other file systems.
We can create a fresh workspace to test multiple file systems in parallel.
The script `workspace/bundle` is used.

```bash
TEST_DIR=<A testing directory>
FS=<File system to test>
./bundle $TEST_DIR
```

Now, we can use dogfood as mentioned before.
Start the QEMU and testing.

```bash
cd $TEST_DIR

$qemu: ./ctrl start # start QEMU

$test: cd $TEST_DIR
$test: ./ctrl init
$test: ./food --fs=$FS --test
```

If you do not want to start qemu manually, using

```bash
$test: ./food --fs=$FS --test --qemu
```

This will start QEMU as a daemon thread automatically,
but the logs/outputs are also not displayed in the terminal.
We must check the log `vm.log` to analyze the results.

### Notes for multiple testing in parallel

When testing multiple file systems in parallel,
you should use `bundle` to create testing directories for each file system and edit file `$TEST_DIR/mngr-tools/config.sh`.

You MUST set `TELNET_PORT`, `TCP_PORT`, and `SSH_PORT` differently for each file system testing.

### Analyze the results

```bash
$test: ./analyze-log.py # Count #crashes
$test: ./analyze-log.py | sort | uniq # Count unique bugs
```

## Reuse Dogfood: how to reproduce a new bug

When finding a bug logged in the log file `workspace/vm.log`,
you can make a test case to reproduce it.
The script `workspace/bug-extract` is used to extract bug cases automatically.
When finding a bug in the log, you can make a test case to reproduce it.
The following section introduces manual steps to reproduce a bug:

### 1. Locate the bug, like

> [  34.786048] kernel BUG at fs/f2fs/data.c:317!

### 2. Check the log from the bug location backward to find the test case name, like

> TEST CASE[ ../workspace/C-output/2019_07_31-20:06:15-cGKYX/2.c with f2fs-0.img ]

This is the C program file to manifest the bug and its disk image.

### 3. Collect all mount options in `MOUNT OPT [*]` between the location of test case name and bug location, like

> MOUNT OPT [ background_gc=off,nouser_xattr,noacl,active_logs=2,inline_data,inline_dentry,extent_cache,noextent_cache,noinline_data,data_flush,mode=lfs,io_bits=3,usrquota,grpquota,quota,noquota,fsync_mode=nobarrier ]

### 4. Copy the C program file and disk image found in step 2 to the directory `template`

### 5. Change the script `template/trigger.sh`

- `DISK_NAME` is the name of disk image with out extension, like `f2fs-0` in our example.
- `CASE_NAME` is the name of C program file, like `2.c` in our example.
- `FS` is the file system to test

### 6. Copy the mount options line by line (the order should be the same as the log file) in the file `template/mountfood`, like

```shell
options=(
"..." # Other options
"background_gc=off,nouser_xattr,noacl,active_logs=2,inline_data,inline_dentry,extent_cache,noextent_cache,noinline_data,data_flush,mode=lfs,io_bits=3,usrquota,grpquota,quota,noquota,fsync_mode=nobarrier"
"..." # Other options
)
```

### 7. Run it as the demo in section Test QEMU

```bash
$test: ./template/trigger.sh
```
