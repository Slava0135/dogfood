/* #define _GNU_SOURCE */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <dirent.h>

#include <string>

#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/xattr.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <fcntl.h>

#include "executor.h"
#include "utils.h"

extern const char *g_workspace;

extern char* g_buffers[];

static const char* patch_path(const char *path) {
    if (path[0] != '/') {
        DPRINTF("Error path: %s\n", path);
        assert(0);
    }

    int len = strlen(g_workspace) + strlen(path) + 10;
    char *result = (char*)malloc(sizeof(char) * len);

    len = snprintf(result, len, "%s%s", g_workspace, path);
    result[len] = '\0';
    return result;
}

// -----------------------------------------------
static int g_nr_failure = 0;
static int g_nr_success = 0;

void report_result() {
    printf("#SUCCESS %d; #FAILURE %d\n", g_nr_success, g_nr_failure);
}

#define SUCCESS_REPORT() \
    do { \
        g_nr_success += 1; \
    } while (0)

#define FAILURE_REPORT(cmd, path) \
    do { \
        DPRINTF("%s FAIL [%s: %d] %s (%s)\n", cmd, file, line, path, strerror(errno)); \
        g_nr_failure += 1; \
    } while (0)


#define FAILURE_REPORT_2(cmd, path_1, path_2) \
    do { \
        DPRINTF("%s FAIL [%s: %d] %s X %s (%s)\n", cmd, file, line, path_1, path_2, strerror(errno)); \
        g_nr_failure += 1; \
    } while (0)


// -----------------------------------------------

#define DEF_FUNC(cmd_name, ...) \
    int impl_do_##cmd_name(__VA_ARGS__, const char *file, int line)

// -----------------------------------------------

DEF_FUNC(mkdir, const char *path, mode_t param) {
    int status = mkdir(patch_path(path), param);
    if (status == -1) {
        FAILURE_REPORT("MKDIR", path);
    }
    append_trace(status, 0);
    SUCCESS_REPORT();
    return status;
}

// -----------------------------------------------

DEF_FUNC(create, const char *path, mode_t param) {
    int status = creat(patch_path(path), param);
    if (status == -1) {
        FAILURE_REPORT("CREATE", path);
    }
    append_trace(status, 0);
    SUCCESS_REPORT();
    return status;
}

// -----------------------------------------------

DEF_FUNC(symlink, const char *old_path, const char *new_path) {
    int status = symlink(patch_path(old_path), patch_path(new_path));
    if (status == -1) {
        FAILURE_REPORT_2("SYMLINK", old_path, new_path);
    }
    append_trace(status, errno);
    SUCCESS_REPORT();
    return status;
}

// -----------------------------------------------

DEF_FUNC(hardlink, const char *old_path, const char *new_path) {
    int status = link(patch_path(old_path), patch_path(new_path));
    if (status == -1) {
        FAILURE_REPORT_2("HARDLINK", old_path, new_path);
    }
    append_trace(status, errno);
    SUCCESS_REPORT();
    return status;
}

// -----------------------------------------------

static int remove_dir(const char *path) {
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;

    if (d) {
        struct dirent *p;
        r = 0;

        while (!r && (p = readdir(d))) {
            int r2 = -1;
            char *buf;
            size_t len;

            /* Skip the names "." and ".." as we don't want to recurse on them.
             */
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
                continue;
            }

            len = path_len + strlen(p->d_name) + 2;
            buf = (char*)malloc(len);

            if (buf) {
                struct stat statbuf;

                snprintf(buf, len, "%s/%s", path, p->d_name);

                if (!lstat(buf, &statbuf)) {
                    if (S_ISDIR(statbuf.st_mode)) {
                        r2 = remove_dir(buf);
                    } else {
                        r2 = unlink(buf);
                        if (r2) {
                            DPRINTF("UNLINK failure %s\n", buf);
                        }
                    }
                }
                free(buf);
            }
            r = r2;
        }
        closedir(d);
    }

    if (!r) {
        r = rmdir(path);
    }

    if (r) {
        DPRINTF("RMDIR failure %s\n", path);
    }

    SUCCESS_REPORT();
    return r;
}

DEF_FUNC(remove, const char *path) {
    path = patch_path(path);
    struct stat file_stat;
    int status = 0;

    status = lstat(path, &file_stat);
    if (status < 0) {
        FAILURE_REPORT("STAT", path);
        append_trace(status, errno);
        return -1;
    }

    if (S_ISDIR(file_stat.st_mode)) {
        status = remove_dir(path);
        if (status) {
            FAILURE_REPORT("RMDIR", path);
        }
    } else {
        status = unlink(path);
        if (status == -1) {
            FAILURE_REPORT("UNLINK", path);
        }
    }

    append_trace(status, errno);
    SUCCESS_REPORT();
    return status;
}

// -----------------------------------------------

DEF_FUNC(open, int *fd_ptr, const char *path, int param) {
    int fd = open(patch_path(path), param);
    if (fd == -1) {
        FAILURE_REPORT("OPEN", path);
    }
    append_trace(fd, errno);
    SUCCESS_REPORT();

    *fd_ptr = fd;
    return fd;
}

// -----------------------------------------------

DEF_FUNC(open_tmpfile, int *fd_ptr, const char *path, int param) {
    int fd = open(patch_path(path),
                  param | O_TMPFILE,
                  S_IRWXU| S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        FAILURE_REPORT("OPEN_TMPFILE", path);
    }
    append_trace(fd, errno);
    SUCCESS_REPORT();

    *fd_ptr = fd;
    return fd;
}

// -----------------------------------------------

DEF_FUNC(close, int fd) {
    int status = close(fd);
    if (status == -1) {
        FAILURE_REPORT("CLOSE", std::to_string(fd).c_str());
    }
    append_trace(status, errno);
    SUCCESS_REPORT();
    return status;
}

// -----------------------------------------------

DEF_FUNC(read, int fd, int buf_id, int size) {
    assert(0 <= buf_id && buf_id < NR_BUF);
    assert(0 <= size && size <= SIZE_PER_BUF);

    char *buf = g_buffers[buf_id];
    if (!buf) {
        DPRINTF("Buffer not initialized\n");
        return -1;
    }
    int nr = read(fd, buf, size);
    if (nr == -1) {
        FAILURE_REPORT("READ", std::to_string(fd).c_str());
        append_trace(nr, errno);
        return -1;
    }
    append_trace(nr, errno);
    SUCCESS_REPORT();
    return 0;
}

// -----------------------------------------------

DEF_FUNC(write, int fd, int buf_id, int size) {
    assert(0 <= buf_id && buf_id < NR_BUF);
    assert(0 <= size && size <= SIZE_PER_BUF);

    char *buf = g_buffers[buf_id];
    int nr = 0;
    for (int i = 0; i < 10; ++i) {
        nr = write(fd, buf, size);
        if (nr == -1) {
            FAILURE_REPORT("WRITE", std::to_string(fd).c_str());
            append_trace(nr, errno);
            return -1;
        }
    }
    append_trace(nr, errno);
    SUCCESS_REPORT();
    // free(buf);
    return 0;
}

// -----------------------------------------------

DEF_FUNC(rename, const char *old_path, const char *new_path) {
    int status = rename(patch_path(old_path), patch_path(new_path));
    if (status == -1) {
      FAILURE_REPORT_2("RENAME", old_path, new_path);
    }
    append_trace(status, errno);
    SUCCESS_REPORT();
    return status;
}

// -----------------------------------------------

DEF_FUNC(sync, bool is_last) {
    sync();
    SUCCESS_REPORT();
    return 0;
}

// -----------------------------------------------

DEF_FUNC(fsync, int fd, bool is_last) {
    SUCCESS_REPORT();
    return fsync(fd);
}

// -----------------------------------------------

DEF_FUNC(enlarge, const char *path, int size) {
    path = patch_path(path);

    struct stat file_stat;
    int status = stat(path, &file_stat);
    if (status < 0) {
        FAILURE_REPORT("STAT", path);
        append_trace(status, errno);
        return -1;
    }

    if (S_ISREG(file_stat.st_mode)) {
        status = truncate(path, size * 10);
        if (status == -1) {
            FAILURE_REPORT("TRUNCATE", path);
        }
        append_trace(status, errno);
        SUCCESS_REPORT();
        return status;
    } else if (S_ISDIR(file_stat.st_mode)) {
        for (int i = 0; i < size; ++i) {
            char *child_name = rand_string(10);
            char *child_path = path_join(path, child_name);

            status = mkdir(child_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            if (status == -1) {
                FAILURE_REPORT("MKDIR", child_path);
                free(child_name);
                free(child_path);
                append_trace(status, errno);
                return status;
            }
        }
        append_trace(status, errno);
        SUCCESS_REPORT();
        return 0;
    }
}

// -----------------------------------------------

DEF_FUNC(deepen, const char *path, int depth) {
    char *curr_path = (char*)patch_path(path);
    int status;

    for (int i = 0; i < depth; ++i) {
        char *child_name = rand_string(1);
        char *child_path = path_join(curr_path, child_name);

        status = mkdir(child_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (status == -1) {
            FAILURE_REPORT("MKDIR", child_path);
            free(child_name);
            free(child_path);
            append_trace(status, errno);
            return status;
        }

        free(child_name);
        free(curr_path);
        curr_path = child_path;
    }
    append_trace(status, errno);
    SUCCESS_REPORT();
    return 0;
}

// -----------------------------------------------

DEF_FUNC(reduce, const char *path) {
    path = patch_path(path);

    struct stat file_stat;
    int status = stat(path, &file_stat);
    if (status < 0) {
        FAILURE_REPORT("STAT", path);
        append_trace(status, errno);
        return -1;
    }
    if (S_ISREG(file_stat.st_mode)) {
        int status = truncate(path, 0);
        if (status == -1) {
          FAILURE_REPORT("TRUNCATE", path);
        }
        append_trace(status, errno);
        SUCCESS_REPORT();
        return status;
    } else if (S_ISDIR(file_stat.st_mode)) {
        char cmd[1024];
        snprintf(cmd, 1024, "rm -rf %s/*", path);
        exec_command(cmd);
        SUCCESS_REPORT();
        return 0;
    }
}

// -----------------------------------------------

DEF_FUNC(write_xattr, const char *path, const char *key, const char *value) {
    path = patch_path(path);
    int status = lsetxattr(path, key, value, strlen(value), XATTR_CREATE);
    if (status == EEXIST) {
        status = lsetxattr(path, key, value, strlen(value), XATTR_REPLACE);
    }
    if (status) {
        FAILURE_REPORT("WRITE_XATTR", path);
    }
    append_trace(status, errno);
    SUCCESS_REPORT();
    return status;
}

// -----------------------------------------------

DEF_FUNC(read_xattr, const char *path, const char *key) {
    path = patch_path(path);

    char buf[1024];
    int status = lgetxattr(path, key, buf, 1024);
    if (status == -1) {
        FAILURE_REPORT("READ_XATTR", path);
    }
    append_trace(status, errno);
    SUCCESS_REPORT();
    return 0;
}

// -----------------------------------------------

DEF_FUNC_NOARGS(remount_root) {
    char cmd[1024];

    //
    // Determine which filesystem under testing
    //
    snprintf(cmd, 1024, "df -T %s | awk '{ getline ; print $2 }'", g_workspace);
    const char *fs = exec_command(cmd);

    //
    // Get remount option
    //
    snprintf(cmd, 1024, "./mountfood %s", fs);
    const char *option = exec_command(cmd);

    int kmsg_fd = open("/dev/kmsg", O_RDWR);
    if (kmsg_fd == -1) {
        DPRINTF("Open /dev/kmsg failure: [%s]\n", strerror(errno));
        return -1;
    }
    int l = snprintf(cmd, 1024, "\033[0;33mREMOUNT OPT (line: %d) [ %s ]\033[0m\n", line, option);
    write(kmsg_fd, cmd, l);
    close(kmsg_fd);

    //
    // Find which device
    //
    snprintf(cmd, 1024, "findmnt -n -o SOURCE --target %s", g_workspace);
    const char *dev = exec_command(cmd);

    // snprintf(cmd, 1024, "mount %s %s -o \"remount,%s\"", dev, g_workspace, option);
    DPRINTF("REMOUNT (line: %d) %s: with [remount,%s]\n", line, fs, option);

    int status = mount(NULL, g_workspace, NULL, MS_REMOUNT, option);
    if (status) {
        FAILURE_REPORT("REMOUNT_ROOT", option);
    }
    append_trace(status, errno);

    SUCCESS_REPORT();
    return status;
    // const char *output = exec_command(cmd);
    // DPRINTF("REMOUNT RESULT: %s\n", output);
}

// -----------------------------------------------

DEF_FUNC(statfs, const char *path) {
    path = patch_path(path);

    struct statfs buf;
    int status = statfs(path, &buf);
    if (status) {
        FAILURE_REPORT("STATFS", path);
    }
    append_trace(status, errno);
    SUCCESS_REPORT();
    return status;
}

// -----------------------------------------------

DEF_FUNC(mknod, const char *path, mode_t mode, dev_t dev) {
    path = patch_path(path);

    int status = mknod(path, mode, dev);
    if (status) {
        FAILURE_REPORT("MKNOD", path);
    }
    append_trace(status, errno);
    SUCCESS_REPORT();
    return status;
}

// -----------------------------------------------

static void* worker(void *arg) {
    bind_cpu(1);
    DPRINTF("WORKER\n");
    seq_func_t do_syscall = (seq_func_t)arg;
    do_syscall();
}

/**
 * Return: whether do_syscall_1 succeeds
 */
DEF_FUNC(concurrent, seq_func_t do_syscall_1, seq_func_t do_syscall_2) {
    //
    // Two possible sequences: SEQ_1: syscall_1 -> syscall_2
    //                         SEQ_2: syscall_2 -> syscall_1
    // These two syscalls interfere with each other and
    // syscall_2 never fails in our abstract model.
    // So, the syscall_1 failure means SEQ_2; otherwise, the SEQ_1 executes.
    //
    pthread_t child_thread;
    pthread_create(&child_thread, NULL, worker, (void*)do_syscall_2);

    int status = do_syscall_1();
    pthread_join(child_thread, NULL);
    return status == 0;
}

// -----------------------------------------------

DEF_FUNC(branch, bool cond, seq_func_t true_seq, seq_func_t false_seq) {
    if (cond) {
        return true_seq();
    } else {
        return false_seq();
    }
}
