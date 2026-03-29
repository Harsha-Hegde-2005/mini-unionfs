#include "fs.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>


int unionfs_read(const char *path, char *buf, size_t size, off_t offset,
                 struct fuse_file_info *fi) {
    return -ENOENT;
}


int unionfs_write(const char *path, const char *buf, size_t size, off_t offset,
                  struct fuse_file_info *fi) {
    return -ENOENT;
}


int unionfs_open(const char *path, struct fuse_file_info *fi) {
    return -ENOENT;
}


int unionfs_create(const char *path, mode_t mode,
                   struct fuse_file_info *fi) {
    return -ENOENT;
}


int unionfs_mkdir(const char *path, mode_t mode) {
    return -ENOENT;
}


int unionfs_rmdir(const char *path) {
    return -ENOENT;
}


int unionfs_unlink(const char *path) {
    return -ENOENT;
}


int unionfs_truncate(const char *path, off_t size,
                     struct fuse_file_info *fi) {
    return -ENOENT;
}


int unionfs_utimens(const char *path, const struct timespec ts[2],
                    struct fuse_file_info *fi) {
    return -ENOENT;
}