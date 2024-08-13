// #define _GNU_SOURCE

#ifndef __EXECUTOR_H__
#define __EXECUTOR_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/mount.h>
#include <pthread.h>

#define DPRINTF(...) \
    do {\
        fprintf(stderr, "[%s %d]: ", __FILE__, __LINE__);\
        fprintf(stderr, __VA_ARGS__ );\
    } while(0)

#define NR_BUF 16
#define SIZE_PER_BUF 4096*20

/*
 * When USE_TRACE is set, the return code of each syscall will be logged.
 */
// #define USE_TRACE

#ifdef USE_TRACE
#define append_trace(ret_code, err) \
    append_trace_impl(__LINE__, ret_code, err)

void append_trace_impl(int line, int ret_code, int err);
void dump_trace();

#else
#define append_trace(ret_code, err)
#endif



typedef int (*seq_func_t)();

void init_executor();
int test_syscall();
void report_result();

// -----------------------------------------------

#define DEF_FUNC(cmd_name, ...) \
    int impl_do_##cmd_name(__VA_ARGS__, const char *file, int line)


#define CALL_FUNC(cmd_name, ...) \
        impl_do_##cmd_name(__VA_ARGS__, __FILE__, __LINE__)

// -----------------------------------------------

DEF_FUNC(mkdir, const char *path, mode_t param);

#define do_mkdir(path, param) \
        CALL_FUNC(mkdir, path, param)

// -----------------------------------------------

DEF_FUNC(create, const char *path, mode_t param);

#define do_create(path, param) \
        CALL_FUNC(create, path, param)

// -----------------------------------------------

DEF_FUNC(symlink, const char *old_path, const char *new_path);

#define do_symlink(old_path, new_path) \
        CALL_FUNC(symlink, old_path, new_path)

// -----------------------------------------------

DEF_FUNC(hardlink, const char *old_path, const char *new_path);

#define do_hardlink(old_path, new_path) \
        CALL_FUNC(hardlink, old_path, new_path)

// -----------------------------------------------

DEF_FUNC(remove, const char *path);

#define do_remove(path) \
        CALL_FUNC(remove, path)

// -----------------------------------------------

DEF_FUNC(open, int *fd_ptr, const char *path, int param);

#define do_open(fd, path, param) \
        CALL_FUNC(open, &fd, path, param)

// -----------------------------------------------

DEF_FUNC(open_tmpfile, int *fd_ptr, const char *path, int param);

#define do_open_tmpfile(fd, path, param) \
        CALL_FUNC(open_tmpfile, &fd, path, param)

// -----------------------------------------------

DEF_FUNC(close, int fd);

#define do_close(fd) \
        CALL_FUNC(close, fd)

// -----------------------------------------------

DEF_FUNC(read, int fd, int buf_id, int size);

#define do_read(fd, buf_id, size) \
        CALL_FUNC(read, fd, buf_id, size)

// -----------------------------------------------

DEF_FUNC(write, int fd, int buf_id, int size);

#define do_write(fd, buf_id, size) \
        CALL_FUNC(write, fd, buf_id, size)

// -----------------------------------------------

DEF_FUNC(rename, const char *old_path, const char *new_path);

#define do_rename(old_path, new_path) \
        CALL_FUNC(rename, old_path, new_path)

// -----------------------------------------------

DEF_FUNC(fsync, int fd, bool is_last);

#define do_fsync(fd, is_last) \
        CALL_FUNC(fsync, fd, is_last)

// -----------------------------------------------

DEF_FUNC(enlarge, const char *path, int size);

#define do_enlarge(path, size) \
        CALL_FUNC(enlarge, path, size)

// -----------------------------------------------

DEF_FUNC(deepen, const char *path, int depth);

#define do_deepen(path, depth) \
        CALL_FUNC(deepen, path, depth)

// -----------------------------------------------

DEF_FUNC(reduce, const char *path);

#define do_reduce(path) \
        CALL_FUNC(reduce, path)

// -----------------------------------------------

DEF_FUNC(branch, bool cond, seq_func_t true_seq, seq_func_t false_seq);

#define do_branch(cond, true_seq, false_seq) \
        CALL_FUNC(branch, cond, true_seq, false_seq)

// -----------------------------------------------

DEF_FUNC(write_xattr, const char *path, const char *key, const char *value);

#define do_write_xattr(path, key, value) \
    CALL_FUNC(write_xattr, path, key, value)

// -----------------------------------------------

DEF_FUNC(read_xattr, const char *path, const char *key);

#define do_read_xattr(path, key) \
    CALL_FUNC(read_xattr, path, key)

// -----------------------------------------------

// ===============================================

#define DEF_FUNC_NOARGS(cmd_name) \
    int impl_do_##cmd_name(const char *file, int line)

#define CALL_FUNC_NOARGS(cmd_name) \
        impl_do_##cmd_name(__FILE__, __LINE__)

// -----------------------------------------------

DEF_FUNC_NOARGS(remount_root);

#define do_remount_root() CALL_FUNC_NOARGS(remount_root)

// -----------------------------------------------

DEF_FUNC(sync, bool is_last);

#define do_sync(is_last) \
    CALL_FUNC(sync, is_last)

// -----------------------------------------------

DEF_FUNC(statfs, const char *path);

#define do_statfs(path) \
    CALL_FUNC(statfs, path)

// -----------------------------------------------

DEF_FUNC(mknod, const char *path, mode_t mode, dev_t dev);

#define do_mknod(path, mode, dev) \
    CALL_FUNC(mknod, path, mode, dev)


#endif /* ifndef __EXECUTOR_H__ */
