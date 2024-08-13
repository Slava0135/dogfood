#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <dirent.h>
#include <cstring>
#include <errno.h>
#include <attr/xattr.h>

#include "../BaseTestCase.h"
#include "../../user_tools/api/workload.h"
#include "../../user_tools/api/actions.h"

using fs_testing::tests::DataTestResult;
using fs_testing::user_tools::api::WriteData;
using fs_testing::user_tools::api::WriteDataMmap;
using fs_testing::user_tools::api::Checkpoint;
using std::string;

#define NR_BUF 16
#define SIZE_PER_BUF 4096*20

//
// Alignment: 4K
//
#define ALIGN 4096

void* align_alloc(size_t size) {
    void *ptr = NULL;
    int ret = posix_memalign(&ptr, ALIGN, size);
    if (ret) {
        fprintf(stderr, "** %s\n", strerror(errno));
    }
    return ptr;
}

char* path_join(const char *prefix, const char *file_name) {
    int len = strlen(prefix) + strlen(file_name) + 3;
    char *buf = (char*)malloc(len);
    int ret = snprintf(buf, len, "%s/%s", prefix, file_name);
    if (ret == len) {
        free(buf);
        fprintf(stderr, "PATH_JOIN failure\n");
        return NULL;
    } else {
        buf[ret] = '\0';
    }
    return buf;
}

char* rand_string(size_t len) {
    static char charset[] = "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";
    if (!len) {
        return NULL;
    }

    char *buf = (char*)align_alloc(len + 1);
    if (!buf) {
        return NULL;
    }

    for (size_t i = 0; i < len; i++) {
        int key = rand() % (int) (sizeof(charset) -1);
        buf[i] = charset[key];
    }

    buf[len] = '\0';
    return buf;
}


namespace fs_testing {
    namespace tests {

        class Example: public BaseTestCase {
            public:
                Example() {
                    local_checkpoint = 0;

                    for (int i = 0; i < NR_BUF; ++i) {
                        char *buf = (char*)align_alloc(SIZE_PER_BUF);
                        if (!buf) {
                            fprintf(stderr, "Malloc failure\n");
                            exit(1);
                        }
                        memset(buf, 'a' + i, SIZE_PER_BUF);
                        buffers[i] = buf;
                    }

                }

                virtual int setup() override {
                    return 0;
                }

                virtual int run( int checkpoint ) override;

                virtual int check_test( unsigned int last_checkpoint, DataTestResult *test_result) override {

                    return 0;
                }

            private:
                int reduce_impl(const char *path);
                int deepen_impl(const char *path, int depth);
                int enlarge_impl(const char *path, int size);
                int fallocate_impl(const char *path, int mode, int offset, int size);
                const char* patch_path(const char *path);

                int local_checkpoint;
                char* buffers[NR_BUF];
        };

    }  // namespace tests
}  // namespace fs_testing

extern "C" fs_testing::tests::BaseTestCase *test_case_get_instance() {
    return new fs_testing::tests::Example;
}

extern "C" void test_case_delete_instance(fs_testing::tests::BaseTestCase *tc) {
    delete tc;
}

namespace fs_testing {
    namespace tests {
        const char* Example::patch_path(const char *path) {
            assert(path[0] == '/');

            int len = strlen(mnt_dir_.c_str()) + strlen(path) + 10;
            char *result = (char*)malloc(sizeof(char) * len);

            len = snprintf(result, len, "%s%s", mnt_dir_.c_str(), path);
            result[len] = '\0';
            return result;
        }

    }  // namespace tests
}  // namespace fs_testing

// ===============================================

#define do_sync(is_last) \
    do { \
        cm_->CmSync();\
        if (cm_->CmCheckpoint() < 0) { \
            return -1; \
        } \
        local_checkpoint += 1; \
        if (local_checkpoint == checkpoint) { \
            if (is_last) { \
                return 1; \
            } else { \
                return 0; \
            } \
        } \
    } while (0)

#define do_fsync(fd, is_last) \
    do { \
        cm_->CmFsync(fd);\
        if (cm_->CmCheckpoint() < 0) { \
            return -1; \
        } \
        local_checkpoint += 1; \
        if (local_checkpoint == checkpoint) { \
            if (is_last) { \
                return 1; \
            } else { \
                return 0; \
            }\
        } \
    } while (0)

#define do_xsync(path, is_last) \
    do { \
        char *real_path = (char*)patch_path(path); \
        int fd = cm_->CmOpen(real_path, O_RDONLY); \
        if (fd < 0) { \
            close(fd); \
            return errno; \
        } \
        cm_->CmFsync(fd);\
        if (cm_->CmCheckpoint() < 0) { \
            return -1; \
        } \
        close(fd); \
        local_checkpoint += 1; \
        if (local_checkpoint == checkpoint) { \
            if (is_last) { \
                return 1; \
            } else { \
                return 0; \
            }\
        } \
    } while (0)

#define do_mkdir(path, mode) \
    do { \
        char *real_path = (char*)patch_path(path); \
        if (cm_->CmMkdir(real_path, mode) < 0) { \
            return errno; \
        } \
    } while (0)

#define do_create(path, mode) \
    do { \
        char *real_path = (char*)patch_path(path); \
        int fd = open(real_path, O_RDWR|O_CREAT, mode); \
        if (fd < 0) { \
            close(fd); \
            return errno; \
        } \
        close(fd); \
    } while (0)

#define do_mknod(path, mode, dev) \
    do { \
        char *real_path = (char*)patch_path(path); \
        if (cm_->CmMknod(real_path, mode, dev) < 0) { \
            return errno; \
        } \
    } while (0)

#define do_symlink(old_path, new_path) \
    do { \
        char *real_old_path = (char*)patch_path(old_path); \
        char *real_new_path = (char*)patch_path(new_path); \
        if (symlink(real_old_path, real_new_path) < 0) { \
            return errno; \
        } \
    } while (0)

#define do_hardlink(old_path, new_path) \
    do { \
        char *real_old_path = (char*)patch_path(old_path); \
        char *real_new_path = (char*)patch_path(new_path); \
        if (link(real_old_path, real_new_path) < 0) { \
            return errno; \
        } \
    } while (0)

#define do_rename(old_path, new_path) \
    do { \
        char *real_old_path = (char*)patch_path(old_path); \
        char *real_new_path = (char*)patch_path(new_path); \
        if (cm_->CmRename(real_old_path, real_new_path) < 0) { \
            return errno; \
        } \
    } while (0)

#define do_open(fd, path, mode) \
    do { \
        fd = cm_->CmOpen((char*)patch_path(path), mode); \
        if (fd < 0) { \
            return errno; \
        } \
    } while (0)

#define do_close(fd)\
    do { \
        if (cm_->CmClose(fd) < 0) { \
            return errno; \
        } \
    } while (0)

#define do_read(fd, buf_id, size) \
    do { \
        if (read(fd, buffers[buf_id], size) < 0) { \
            return errno; \
        } \
    } while (0)

#define do_write(fd, buf_id, size) \
    do { \
        if (cm_->CmWrite(fd, buffers[buf_id], size) < 0) { \
            return errno; \
        } \
    } while (0)

namespace fs_testing {
    namespace tests {
        int Example::reduce_impl(const char *path) {
            struct stat file_stat;
            int status = stat(path, &file_stat);
            if (status < 0) {
                return -1;
            }
            if (S_ISREG(file_stat.st_mode)) {
                status = truncate(path, 0);
                // int fd = cm_->CmOpen(path, O_RDWR);
                // if (fd == -1) {
                    // return errno;
                // }
                // status = fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, rand() % 1024, 0);
                // cm_->CmClose(fd);
                // if (status < 0) {
                    // return errno;
                // }
                return status;
            } else if (S_ISDIR(file_stat.st_mode)) {
                // char cmd[1024];
                // snprintf(cmd, 1024, "rm -rf %s/*", path);
                // system(cmd);
                return 0;
            }
        }
    }
}

#define do_reduce(path) \
    do { \
        char *real_path = (char*)patch_path(path); \
        if (reduce_impl(real_path) < 0) { \
            return errno; \
        } \
    } while (0)

namespace fs_testing {
    namespace tests {
        int Example::deepen_impl(const char *path, int depth) {
            int status;
            char *curr_path = (char*)path;

            for (int i = 0; i < depth; ++i) {
                char *child_name = rand_string(1);
                char *child_path = path_join(curr_path, child_name);

                status = cm_->CmMkdir(child_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                if (status == -1) {
                    free(child_name);
                    free(child_path);
                    return status;
                }

                free(child_name);
                free(curr_path);
                curr_path = child_path;
            }
            return 0;
        }
    }
}


#define do_deepen(path, depth) \
    do { \
        char *real_path = (char*)patch_path(path); \
        if (deepen_impl(real_path, depth) < 0) { \
            return errno; \
        } \
    } while (0)

namespace fs_testing {
    namespace tests {
        int Example::enlarge_impl(const char *path, int size) {
            struct stat file_stat;
            int status = stat(path, &file_stat);
            if (status < 0) {
                return -1;
            }

            if (S_ISREG(file_stat.st_mode)) {
                status = truncate(path, size * 5);
                // int fd = cm_->CmOpen(path, O_RDWR);
                // if (fd == -1) {
                    // return errno;
                // }
                // status = fallocate(fd, 0, rand() % 1024, size);
                // cm_->CmClose(fd);
                // if (status < 0) {
                    // return errno;
                // }
                return status;
            } else if (S_ISDIR(file_stat.st_mode)) {
                // for (int i = 0; i < size; ++i) {
                    // char *child_name = rand_string(10);
                    // char *child_path = path_join(path, child_name);

                    // status = cm_->CmMkdir(child_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                    // if (status == -1) {
                        // free(child_name);
                        // free(child_path);
                        // return status;
                    // }
                // }
                return 0;
            }
        }
    }
}

#define do_enlarge(path, size) \
    do { \
        char *real_path = (char*)patch_path(path); \
        if (enlarge_impl(real_path, size) < 0) { \
            return errno; \
        } \
    } while (0)


namespace fs_testing {
    namespace tests {
        int Example::fallocate_impl(const char *path, int mode, int offset, int size) {
            int fd = cm_->CmOpen(path, O_RDWR);
            if (fd == -1) {
                return errno;
            }
            int status = fallocate(fd, mode, offset, size);
            cm_->CmClose(fd);
            if (status < 0) {
                return errno;
            }
            return status;
       }
    }
}

#define do_fallocate(path, mode, offset, size) \
    do { \
        char *real_path = (char*)patch_path(path); \
        if (fallocate_impl(real_path, mode, offset, size) < 0) { \
            return errno; \
        } \
    } while (0)

#define do_remove(path) \
    do { \
        char *real_path = (char*)patch_path(path); \
        char cmd[1024]; \
        snprintf(cmd, 1024, "rm -rf %s", real_path); \
        system(cmd); \
    } while (0)

#define do_statfs(path) \
    do { \
    } while (0)

#define do_remount_root() \
    do { \
    } while (0)


// ===============================================

// namespace fs_testing {
// namespace tests {

// int Example::run( int checkpoint ) {
// return 0;
// }

// }  // namespace tests
// }  // namespace fs_testing

