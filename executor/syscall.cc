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
        DPRINTF("ERROR: when patching path %s\n", path);
        assert(0);
    }

    int len = strlen(g_workspace) + strlen(path) + 10;
    char *result = (char*)malloc(sizeof(char) * len);

    len = snprintf(result, len, "%s%s", g_workspace, path);
    result[len] = '\0';
    return result;
}

static int failure_n = 0;
static int success_n = 0;

static void success(int status) {
    append_trace(status, 0);
    success_n += 1;
}

static void failure(int status, const char* cmd, const char* path) {
    append_trace(status, errno);
    DPRINTF("%s FAIL %s (%s)\n", cmd, path, strerror(errno));
    failure_n += 1;
}

static void failure2(int status, const char* cmd, const char* path1, const char* path2) {
    append_trace(status, errno);
    DPRINTF("%s FAIL %s X %s (%s)\n", cmd, path1, path2, strerror(errno));
    failure_n += 1;
}

void report_result() {
    printf("#SUCCESS %d; #FAILURE %d\n", success_n, failure_n);
}

int do_mkdir(const char *path, mode_t param) {
    int status = mkdir(patch_path(path), param);
    if (status == -1) {
        failure(status, "MKDIR", path);
    } else {
        success(status);
    }
    return status;
}

int do_create(const char *path, mode_t param) {
    int status = creat(patch_path(path), param);
    if (status == -1) {
        failure(status, "CREATE", path);
    } else {
        success(status);
    }
    return status;
}

int do_symlink(const char *old_path, const char *new_path) {
    int status = symlink(patch_path(old_path), patch_path(new_path));
    if (status == -1) {
        failure2(status, "SYMLINK", old_path, new_path);
    } else {
        success(status);
    }
    return status;
}

int do_hardlink(const char *old_path, const char *new_path) {
    int status = link(patch_path(old_path), patch_path(new_path));
    if (status == -1) {
        failure2(status, "HARDLINK", old_path, new_path);
    } else {
        success(status);
    }
    return status;
}

static int remove_dir(const char *path) {
    DIR *d = opendir(path);
    std::size_t path_len = strlen(path);
    int r = -1;

    if (d) {
        struct dirent *p;
        r = 0;

        while (!r && (p = readdir(d))) {
            int r2 = -1;
            char *buf;
            std::size_t len;

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
                            DPRINTF("ERROR: unlink failure %s\n", buf);
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
        DPRINTF("ERROR: rmdir failure %s\n", path);
    }

    return r;
}

int do_remove(const char *path) {
    path = patch_path(path);
    struct stat file_stat;
    int status = 0;

    status = lstat(path, &file_stat);
    if (status < 0) {
        failure(status, "STAT", path);
        return -1;
    }

    if (S_ISDIR(file_stat.st_mode)) {
        status = remove_dir(path);
        if (status) {
            failure(status, "RMDIR", path);
        } else {
            success(status);
        }
    } else {
        status = unlink(path);
        if (status == -1) {
            failure(status, "UNLINK", path);
        } else {
            success(status);
        }
    }

    return status;
}

int do_open(int &fd, const char *path, int param) {
    fd = open(patch_path(path), param);
    if (fd == -1) {
        failure(fd, "OPEN", path);
    } else {
        success(fd);
    }
    return fd;
}

int do_open_tmpfile(int &fd, const char *path, int param) {
    fd = open(patch_path(path), param | O_TMPFILE, S_IRWXU| S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        failure(fd, "OPEN_TMPFILE", path);
    } else {
        success(fd);
    }
    return fd;
}

int do_close(int fd) {
    int status = close(fd);
    if (status == -1) {
        failure(status, "CLOSE", std::to_string(fd).c_str());
    } else {
        success(status);
    }
    return status;
}

int do_read(int fd, int buf_id, int size) {
    assert(0 <= buf_id && buf_id < NR_BUF);
    assert(0 <= size && size <= SIZE_PER_BUF);

    char *buf = g_buffers[buf_id];
    if (!buf) {
        DPRINTF("ERROR: buffer not initialized\n");
        return -1;
    }

    int nr = read(fd, buf, size);
    if (nr == -1) {
        failure(nr, "READ", std::to_string(fd).c_str());
        return -1;
    } else {
        success(nr);
    }
    return 0;
}

int do_write(int fd, int buf_id, int size) {
    assert(0 <= buf_id && buf_id < NR_BUF);
    assert(0 <= size && size <= SIZE_PER_BUF);

    char *buf = g_buffers[buf_id];
    int nr = 0;
    for (int i = 0; i < 10; ++i) {
        nr = write(fd, buf, size);
        if (nr == -1) {
            failure(nr, "WRITE", std::to_string(fd).c_str());
            return -1;
        }
    }
    success(nr);
    return 0;
}

int do_rename(const char *old_path, const char *new_path) {
    int status = rename(patch_path(old_path), patch_path(new_path));
    if (status == -1) {
        failure2(status, "RENAME", old_path, new_path);
    } else {
        success(status);
    }
    return status;
}

int do_sync(bool is_last) {
    sync();
    success(0);
    return 0;
}

int do_fsync(int fd, bool is_last) {
    int status = fsync(fd);
    if (status == -1) {
        failure(status, "FSYNC", (std::string() + "<" + std::to_string(fd) + ">").c_str());
    } else {
        success(status);
    }
    return status;
}

int do_enlarge(const char *path, int size) {
    path = patch_path(path);

    struct stat file_stat;
    int status = stat(path, &file_stat);
    if (status < 0) {
        failure(status, "STAT", path);
        return -1;
    }

    if (S_ISREG(file_stat.st_mode)) {
        status = truncate(path, size * 10);
        if (status == -1) {
            failure(status, "TRUNCATE", path);
        } else {
            success(status);
        }
        return status;
    } else if (S_ISDIR(file_stat.st_mode)) {
        for (int i = 0; i < size; ++i) {
            char *child_name = rand_string(10);
            char *child_path = path_join(path, child_name);

            status = mkdir(child_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            if (status == -1) {
                failure(status, "MKDIR", child_path);
                free(child_name);
                free(child_path);
                return status;
            }
        }
        success(status);
        return 0;
    }
}

int do_deepen(const char *path, int depth) {
    char *curr_path = (char*)patch_path(path);
    int status;

    for (int i = 0; i < depth; ++i) {
        char *child_name = rand_string(1);
        char *child_path = path_join(curr_path, child_name);

        status = mkdir(child_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (status == -1) {
            failure(status, "MKDIR", child_path);
            free(child_name);
            free(child_path);
            return status;
        }

        free(child_name);
        free(curr_path);
        curr_path = child_path;
    }
    success(status);
    return 0;
}

int do_reduce(const char *path) {
    path = patch_path(path);

    struct stat file_stat;
    int status = stat(path, &file_stat);
    if (status < 0) {
        failure(status, "STAT", path);
        return -1;
    }
    if (S_ISREG(file_stat.st_mode)) {
        int status = truncate(path, 0);
        if (status == -1) {
            failure(status, "TRUNCATE", path);
        } else {
            success(status);
        }
        return status;
    } else if (S_ISDIR(file_stat.st_mode)) {
        char cmd[1024];
        snprintf(cmd, 1024, "rm -rf %s/*", path);
        exec_command(cmd);
        success(0);
        return 0;
    }
}

int do_write_xattr(const char *path, const char *key, const char *value) {
    path = patch_path(path);
    int status = lsetxattr(path, key, value, strlen(value), XATTR_CREATE);
    if (status == EEXIST) {
        status = lsetxattr(path, key, value, strlen(value), XATTR_REPLACE);
    }
    if (status) {
        failure(status, "WRITE_XATTR", path);
    } else {
        success(status);
    }
    return status;
}

int do_read_xattr(const char *path, const char *key) {
    path = patch_path(path);

    char buf[1024];
    int status = lgetxattr(path, key, buf, 1024);
    if (status == -1) {
        failure(status, "READ_XATTR", path);
    } else {
        success(status);
    }
    return status;
}

int do_statfs(const char *path) {
    path = patch_path(path);

    struct statfs buf;
    int status = statfs(path, &buf);
    if (status) {
        failure(status, "STATFS", path);
    } else {
        success(status);
    }
    return status;
}

int do_mknod(const char *path, mode_t mode, dev_t dev) {
    path = patch_path(path);

    int status = mknod(path, mode, dev);
    if (status) {
        failure(status, "MKNOD", path);
    } else {
        success(status);
    }
    return status;
}
