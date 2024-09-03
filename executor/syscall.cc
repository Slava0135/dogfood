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

static std::string patch_path(const std::string& path) {
    if (path[0] != '/') {
        DPRINTF("ERROR: when patching path %s\n", path.c_str());
        exit(0);
    }
    return g_workspace + path;
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
    int status = mkdir(patch_path(path).c_str(), param);
    if (status == -1) {
        failure(status, "MKDIR", path);
    } else {
        success(status, "MKDIR");
    }
    return status;
}

int do_create(const char *path, mode_t param) {
    idx++;
    int status = creat(patch_path(path).c_str(), param);
    if (status == -1) {
        failure(status, "CREATE", path);
    } else {
        success(status, "CREATE");
    }
    return status;
}

int do_symlink(const char *old_path, const char *new_path) {
    idx++;
    int status = symlink(patch_path(old_path).c_str(), patch_path(new_path).c_str());
    if (status == -1) {
        failure2(status, "SYMLINK", old_path, new_path);
    } else {
        success(status, "SYMLINK");
    }
    return status;
}

int do_hardlink(const char *old_path, const char *new_path) {
    idx++;
    int status = link(patch_path(old_path).c_str(), patch_path(new_path).c_str());
    if (status == -1) {
        failure2(status, "HARDLINK", old_path, new_path);
    } else {
        success(status, "HARDLINK");
    }
    return status;
}

static int remove_dir(const char *p) {
    const std::string dir_path(p);
    DIR *d = opendir(dir_path.c_str());
    int status = -1;

    if (d) {
        struct dirent *p;
        status = 0;

        while (!status && (p = readdir(d))) {
            // Skip the names "." and ".." as we don't want to recurse on them.
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
                continue;
            }

            struct stat statbuf;
            int status_in_dir = -1;            
            const std::string file_path = path_join(dir_path, p->d_name);

            if (!lstat(file_path.c_str(), &statbuf)) {
                if (S_ISDIR(statbuf.st_mode)) {
                    status_in_dir = remove_dir(file_path.c_str());
                } else {
                    status_in_dir = unlink(file_path.c_str());
                    if (status_in_dir) {
                        DPRINTF("ERROR: unlink failure %s\n", file_path.c_str());
                    }
                }
            }
            status = status_in_dir;
        }
        closedir(d);
    }

    if (!status) {
        status = rmdir(dir_path.c_str());
    } else {
        DPRINTF("ERROR: rmdir failure %s\n", dir_path.c_str());
    }

    return status;
}

int do_remove(const char *p) {
    idx++;
    const std::string path = patch_path(p);
    struct stat file_stat;
    int status = 0;

    status = lstat(path.c_str(), &file_stat);
    if (status < 0) {
        failure(status, "STAT", path.c_str());
        return -1;
    }

    if (S_ISDIR(file_stat.st_mode)) {
        status = remove_dir(path.c_str());
        if (status) {
            failure(status, "RMDIR", path.c_str());
        } else {
            success(status, "RMDIR");
        }
    } else {
        status = unlink(path.c_str());
        if (status == -1) {
            failure(status, "UNLINK", path.c_str());
        } else {
            success(status, "UNLINK");
        }
    }

    return status;
}

int do_open(int &fd, const char *path, int param) {
    idx++;
    fd = open(patch_path(path).c_str(), param);
    if (fd == -1) {
        failure(fd, "OPEN", path);
    } else {
        success(fd, "OPEN");
    }
    return fd;
}

int do_open_tmpfile(int &fd, const char *path, int param) {
    idx++;
    fd = open(patch_path(path).c_str(), param | O_TMPFILE, S_IRWXU| S_IWUSR | S_IRGRP | S_IROTH);
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

int do_read(int fd, std::size_t buf_id, std::size_t size) {
    idx++;
    assert(buf_id < NR_BUF);
    assert(size <= SIZE_PER_BUF);

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
        return nr;
    }
}

int do_write(int fd, std::size_t buf_id, std::size_t size) {
    idx++;
    assert(buf_id < NR_BUF);
    assert(size <= SIZE_PER_BUF);

    char *buf = g_buffers[buf_id];
    int nr  = write(fd, buf, size);
    if (nr == -1) {
        failure(nr, "WRITE", std::to_string(fd).c_str());
        return -1;
    } else {
        success(nr, "WRITE");
        return nr;
    }
}

int do_rename(const char *old_path, const char *new_path) {
    idx++;
    int status = rename(patch_path(old_path).c_str(), patch_path(new_path).c_str());
    if (status == -1) {
        failure2(status, "RENAME", old_path, new_path);
    } else {
        success(status, "RENAME");
    }
    return status;
}

int do_sync([[maybe_unused]] bool is_last) {
    idx++;
    sync();
    success(0, "SYNC");
    return 0;
}

int do_fsync(int fd, [[maybe_unused]] bool is_last) {
    idx++;
    int status = fsync(fd);
    if (status == -1) {
        failure(status, "FSYNC", (std::string() + "<" + std::to_string(fd) + ">").c_str());
    } else {
        success(status, "FSYNC");
    }
    return status;
}

int do_enlarge(const char *p, int size) {
    idx++;
    const std::string path = patch_path(p);

    struct stat file_stat;
    int status = stat(path.c_str(), &file_stat);
    if (status < 0) {
        failure(status, "STAT", path.c_str());
        return -1;
    }

    if (S_ISREG(file_stat.st_mode)) {
        status = truncate(path.c_str(), size * 10);
        if (status == -1) {
            failure(status, "TRUNCATE", path.c_str());
        } else {
            success(status, "TRUNCATE");
        }
        return status;
    } else if (S_ISDIR(file_stat.st_mode)) {
        for (int i = 0; i < size; ++i) {
            auto child_name = rand_string(10);
            auto child_path = path_join(path.c_str(), child_name.c_str());
            status = mkdir(child_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            if (status == -1) {
                failure(status, "MKDIR", child_path.c_str());
                return status;
            }
        }
        success(status, "ENLARGE");
        return 0;
    } else {
        DPRINTF("ERROR: unknown file mode: %d", file_stat.st_mode);
        return -1;
    }
}

int do_deepen(const char *p, int depth) {
    idx++;
    std::string curr_path = patch_path(p);
    int status;

    for (int i = 0; i < depth; ++i) {
        auto child_name = rand_string(1);
        auto child_path = path_join(curr_path.c_str(), child_name.c_str());
        status = mkdir(child_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (status == -1) {
            failure(status, "MKDIR", child_path.c_str());
            return status;
        }
        curr_path = child_path;
    }
    success(status, "DEEPEN");
    return 0;
}

int do_reduce(const char *p) {
    idx++;
    const std::string path = patch_path(p);

    struct stat file_stat;
    int status = stat(path.c_str(), &file_stat);
    if (status < 0) {
        failure(status, "STAT", path.c_str());
        return -1;
    }
    if (S_ISREG(file_stat.st_mode)) {
        int status = truncate(path.c_str(), 0);
        if (status == -1) {
            failure(status, "TRUNCATE", path.c_str());
        } else {
            success(status, "TRUNCATE");
        }
        return status;
    } else if (S_ISDIR(file_stat.st_mode)) {
        int status = std::filesystem::remove_all(path);
        if (status == -1) {
            failure(status, "REDUCE", path.c_str());
        } else {
            success(status, "REDUCE");
        }
        return status;
    } else {
        DPRINTF("ERROR: unknown file mode: %d", file_stat.st_mode);
        return -1;
    }
}

int do_write_xattr(const char *p, const char *key, const char *value) {
    idx++;
    const std::string path = patch_path(p);
    int status = lsetxattr(path.c_str(), key, value, strlen(value), XATTR_CREATE);
    if (status == EEXIST) {
        status = lsetxattr(path.c_str(), key, value, strlen(value), XATTR_REPLACE);
    }
    if (status) {
        failure(status, "WRITE_XATTR", path.c_str());
    } else {
        success(status, "WRITE_XATTR");
    }
    return status;
}

int do_read_xattr(const char *p, const char *key) {
    idx++;
    const std::string path = patch_path(p);

    char buf[1024];
    int status = lgetxattr(path.c_str(), key, buf, 1024);
    if (status == -1) {
        failure(status, "READ_XATTR", path.c_str());
    } else {
        success(status, "READ_XATTR");
    }
    return status;
}

int do_statfs(const char *p) {
    idx++;
    const std::string path = patch_path(p);

    struct statfs buf;
    int status = statfs(path.c_str(), &buf);
    if (status) {
        failure(status, "STATFS", path.c_str());
    } else {
        success(status, "STATFS");
    }
    return status;
}

int do_mknod(const char *p, mode_t mode, dev_t dev) {
    idx++;
    const std::string path = patch_path(p);

    int status = mknod(path.c_str(), mode, dev);
    if (status) {
        failure(status, "MKNOD", path.c_str());
    } else {
        success(status, "MKNOD");
    }
    return status;
}
