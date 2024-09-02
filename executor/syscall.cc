#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstddef>
#include <cerrno>
#include <string>
#include <filesystem>

#include <dirent.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
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

static int idx = 0;
static int failure_n = 0;
static int success_n = 0;

static void success(int status, const char* cmd) {
    append_trace(idx, cmd, status, 0);
    success_n += 1;
}

static void failure(int status, const char* cmd, const char* path) {
    append_trace(idx, cmd, status, errno);
    DPRINTF("%s FAIL %s (%s)\n", cmd, path, strerror(errno));
    failure_n += 1;
}

static void failure2(int status, const char* cmd, const char* path1, const char* path2) {
    append_trace(idx, cmd, status, errno);
    DPRINTF("%s FAIL %s X %s (%s)\n", cmd, path1, path2, strerror(errno));
    failure_n += 1;
}

void report_result() {
    printf("#SUCCESS %d; #FAILURE %d\n", success_n, failure_n);
}

int do_mkdir(const char *path, mode_t param) {
    idx++;
    int status = mkdir(patch_path(path), param);
    if (status == -1) {
        failure(status, "MKDIR", path);
    } else {
        success(status, "MKDIR");
    }
    return status;
}

int do_create(const char *path, mode_t param) {
    idx++;
    int status = creat(patch_path(path), param);
    if (status == -1) {
        failure(status, "CREATE", path);
    } else {
        success(status, "CREATE");
    }
    return status;
}

int do_symlink(const char *old_path, const char *new_path) {
    idx++;
    int status = symlink(patch_path(old_path), patch_path(new_path));
    if (status == -1) {
        failure2(status, "SYMLINK", old_path, new_path);
    } else {
        success(status, "SYMLINK");
    }
    return status;
}

int do_hardlink(const char *old_path, const char *new_path) {
    idx++;
    int status = link(patch_path(old_path), patch_path(new_path));
    if (status == -1) {
        failure2(status, "HARDLINK", old_path, new_path);
    } else {
        success(status, "HARDLINK");
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
    idx++;
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
            success(status, "RMDIR");
        }
    } else {
        status = unlink(path);
        if (status == -1) {
            failure(status, "UNLINK", path);
        } else {
            success(status, "UNLINK");
        }
    }

    return status;
}

int do_open(int &fd, const char *path, int param) {
    idx++;
    fd = open(patch_path(path), param);
    if (fd == -1) {
        failure(fd, "OPEN", path);
    } else {
        success(fd, "OPEN");
    }
    return fd;
}

int do_open_tmpfile(int &fd, const char *path, int param) {
    idx++;
    fd = open(patch_path(path), param | O_TMPFILE, S_IRWXU| S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        failure(fd, "OPEN_TMPFILE", path);
    } else {
        success(fd, "OPEN_TMPFILE");
    }
    return fd;
}

int do_close(int fd) {
    idx++;
    int status = close(fd);
    if (status == -1) {
        failure(status, "CLOSE", std::to_string(fd).c_str());
    } else {
        success(status, "CLOSE");
    }
    return status;
}

int do_read(int fd, int buf_id, int size) {
    idx++;
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
        success(nr, "READ");
    }
    return 0;
}

int do_write(int fd, int buf_id, int size) {
    idx++;
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
    success(nr, "WRITE");
    return 0;
}

int do_rename(const char *old_path, const char *new_path) {
    idx++;
    int status = rename(patch_path(old_path), patch_path(new_path));
    if (status == -1) {
        failure2(status, "RENAME", old_path, new_path);
    } else {
        success(status, "RENAME");
    }
    return status;
}

int do_sync(bool is_last) {
    sync();
    success(0, "SYNC");
    return 0;
}

int do_fsync(int fd, bool is_last) {
    idx++;
    int status = fsync(fd);
    if (status == -1) {
        failure(status, "FSYNC", (std::string() + "<" + std::to_string(fd) + ">").c_str());
    } else {
        success(status, "FSYNC");
    }
    return status;
}

int do_enlarge(const char *path, int size) {
    idx++;
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
            success(status, "TRUNCATE");
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
        success(status, "ENLARGE");
        return 0;
    }
}

int do_deepen(const char *path, int depth) {
    idx++;
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
    success(status, "DEEPEN");
    return 0;
}

int do_reduce(const char *path) {
    idx++;
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
            success(status, "TRUNCATE");
        }
        return status;
    } else if (S_ISDIR(file_stat.st_mode)) {
        int status = std::filesystem::remove_all(path);
        if (status == -1) {
            failure(status, "REDUCE", path);
        } else {
            success(status, "REDUCE");
        }
        return status;
    }
}

int do_write_xattr(const char *path, const char *key, const char *value) {
    idx++;
    path = patch_path(path);
    int status = lsetxattr(path, key, value, strlen(value), XATTR_CREATE);
    if (status == EEXIST) {
        status = lsetxattr(path, key, value, strlen(value), XATTR_REPLACE);
    }
    if (status) {
        failure(status, "WRITE_XATTR", path);
    } else {
        success(status, "WRITE_XATTR");
    }
    return status;
}

int do_read_xattr(const char *path, const char *key) {
    idx++;
    path = patch_path(path);

    char buf[1024];
    int status = lgetxattr(path, key, buf, 1024);
    if (status == -1) {
        failure(status, "READ_XATTR", path);
    } else {
        success(status, "READ_XATTR");
    }
    return status;
}

int do_statfs(const char *path) {
    idx++;
    path = patch_path(path);

    struct statfs buf;
    int status = statfs(path, &buf);
    if (status) {
        failure(status, "STATFS", path);
    } else {
        success(status, "STATFS");
    }
    return status;
}

int do_mknod(const char *path, mode_t mode, dev_t dev) {
    idx++;
    path = patch_path(path);

    int status = mknod(path, mode, dev);
    if (status) {
        failure(status, "MKNOD", path);
    } else {
        success(status, "MKNOD");
    }
    return status;
}
