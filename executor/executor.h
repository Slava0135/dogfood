#pragma once

#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#define DPRINTF(...) \
    do {\
        fprintf(stderr, "%s:%d: ", __FILE__, __LINE__);\
        fprintf(stderr, __VA_ARGS__ );\
    } while(0)

const std::size_t NR_BUF = 16;
const std::size_t ALIGN = 4096;
const std::size_t SIZE_PER_BUF = ALIGN*20;

/*
 * When USE_TRACE is set, the return code of each syscall will be logged.
 */
#define USE_TRACE

#ifdef USE_TRACE
#define append_trace(idx, cmd, ret_code, err) \
    append_trace_impl(idx, cmd, ret_code, err)

void append_trace_impl(int idx, const char* cmd, int ret_code, int err);
void dump_trace();

#else
#define append_trace(idx, cmd, ret_code, err)
#endif

int test_syscall();
void report_result();

extern "C" {
    int do_mkdir(const char *path, mode_t param);
    int do_create(const char *path, mode_t param);
    int do_symlink(const char *old_path, const char *new_path);
    int do_hardlink(const char *old_path, const char *new_path);
    int do_remove(const char *path);
    int do_open(int &fd, const char *path, int param);
    int do_open_tmpfile(int &fd, const char *path, int param);
    int do_close(int fd);
    int do_read(int fd, std::size_t buf_id, std::size_t size);
    int do_write(int fd, std::size_t buf_id, std::size_t size);
    int do_rename(const char *old_path, const char *new_path);
    int do_fsync(int fd, bool is_last);
    int do_enlarge(const char *path, int size);
    int do_deepen(const char *path, int depth);
    int do_reduce(const char *path);
    int do_write_xattr(const char *path, const char *key, const char *value);
    int do_read_xattr(const char *path, const char *key);
    int do_sync(bool is_last);
    int do_statfs(const char *path);
    int do_mknod(const char *path, mode_t mode, dev_t dev);
}
