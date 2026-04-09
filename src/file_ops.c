#include "fs.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

/* ============================================================
 * cow_to_upper - Copy a file from lower layer to upper layer.
 *
 * Called before any write or truncate on a lower-layer file.
 * Copies the full file content so subsequent writes apply
 * at correct offsets without corrupting the file.
 * The lower-layer file is NEVER touched.
 * ============================================================ */
static int cow_to_upper(const char *lower_path, const char *upper_path) {
    int src  = open(lower_path, O_RDONLY);
    int dest = open(upper_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (src < 0 || dest < 0) {
        if (src  >= 0) close(src);
        if (dest >= 0) close(dest);
        return -errno;
    }

    char buf[4096];
    ssize_t bytes;
    while ((bytes = read(src, buf, sizeof(buf))) > 0)
        write(dest, buf, bytes);

    close(src);
    close(dest);
    return 0;
}

/* ============================================================
 * unionfs_open - Called when any file is opened.
 *
 * This is the correct place for Copy-on-Write (per FUSE design
 * and Appendix A of the project spec).
 *
 * If the file exists only in lower and is opened for writing,
 * copy it to upper NOW — before any write() call arrives.
 * This guarantees the lower layer is never modified.
 * ============================================================ */
int unionfs_open(const char *path, struct fuse_file_info *fi) {
    struct mini_unionfs_state *state = UNIONFS_DATA;

    char upper_path[PATH_MAX];
    char lower_path[PATH_MAX];
    snprintf(upper_path, PATH_MAX, "%s%s", state->upper_dir, path);
    snprintf(lower_path, PATH_MAX, "%s%s", state->lower_dir, path);

    /* CoW trigger: opened for writing AND only exists in lower */
    if ((fi->flags & O_ACCMODE) != O_RDONLY) {
        if (access(upper_path, F_OK) != 0 &&
            access(lower_path, F_OK) == 0) {
            return cow_to_upper(lower_path, upper_path);
        }
    }
    return 0;
}

/* ============================================================
 * unionfs_read - Read file contents.
 *
 * Resolves path via resolve_path() — returns upper if present,
 * otherwise lower. Whiteout is also checked inside resolve_path.
 * ============================================================ */
int unionfs_read(const char *path, char *buf, size_t size,
                 off_t offset, struct fuse_file_info *fi) {
    char resolved[PATH_MAX];
    if (resolve_path(path, resolved) != 0)
        return -ENOENT;

    int fd = open(resolved, O_RDONLY);
    if (fd < 0) return -errno;

    ssize_t res = pread(fd, buf, size, offset);
    close(fd);
    return (int)res;
}

/* ============================================================
 * unionfs_write - Write data to a file.
 *
 * CoW is already triggered in open(). This function writes
 * directly to the upper-layer file.
 * A fallback CoW is included as a safety net in case open()
 * was somehow bypassed (e.g. direct write without open).
 * ============================================================ */
int unionfs_write(const char *path, const char *buf, size_t size,
                  off_t offset, struct fuse_file_info *fi) {
    struct mini_unionfs_state *state = UNIONFS_DATA;

    char upper_path[PATH_MAX];
    char lower_path[PATH_MAX];
    snprintf(upper_path, PATH_MAX, "%s%s", state->upper_dir, path);
    snprintf(lower_path, PATH_MAX, "%s%s", state->lower_dir, path);

    /* Fallback CoW — safety net */
    if (access(upper_path, F_OK) != 0 &&
        access(lower_path, F_OK) == 0) {
        cow_to_upper(lower_path, upper_path);
    }

    int fd = open(upper_path, O_WRONLY);
    if (fd < 0) return -errno;

    ssize_t res = pwrite(fd, buf, size, offset);
    close(fd);
    return (int)res;
}

/* ============================================================
 * unionfs_truncate - Truncate a file to the given size.
 *
 * Must trigger CoW first if the file lives only in lower,
 * otherwise we would truncate the read-only source.
 * ============================================================ */
int unionfs_truncate(const char *path, off_t size,
                     struct fuse_file_info *fi) {
    struct mini_unionfs_state *state = UNIONFS_DATA;

    char upper_path[PATH_MAX];
    char lower_path[PATH_MAX];
    snprintf(upper_path, PATH_MAX, "%s%s", state->upper_dir, path);
    snprintf(lower_path, PATH_MAX, "%s%s", state->lower_dir, path);

    /* CoW before truncating a lower-layer file */
    if (access(upper_path, F_OK) != 0 &&
        access(lower_path, F_OK) == 0) {
        int ret = cow_to_upper(lower_path, upper_path);
        if (ret != 0) return ret;
    }

    return truncate(upper_path, size);
}

/* ============================================================
 * unionfs_unlink - Delete a file.
 *
 * Two cases:
 *   1. File exists in upper  -> physically delete it from upper
 *   2. File exists in lower  -> create a whiteout marker (.wh.<n>)
 *      so the file is hidden from the merged view
 *
 * A whiteout is always created to safely cover both cases —
 * even if the file was in upper, a lower copy may still exist.
 * ============================================================ */
int unionfs_unlink(const char *path) {
    struct mini_unionfs_state *state = UNIONFS_DATA;

    char upper_path[PATH_MAX];
    snprintf(upper_path, PATH_MAX, "%s%s", state->upper_dir, path);

    /* Delete from upper if it exists there */
    if (access(upper_path, F_OK) == 0)
        unlink(upper_path);

    /* Build whiteout path: upper_dir/subdir/.wh.filename */
    const char *basename = strrchr(path, '/');
    basename = basename ? basename + 1 : path + 1;

    char dir_part[PATH_MAX];
    strncpy(dir_part, path, PATH_MAX);
    char *last_slash = strrchr(dir_part, '/');
    if (last_slash && last_slash != dir_part)
        *last_slash = '\0';
    else
        strcpy(dir_part, "");

    char whiteout[PATH_MAX];
    snprintf(whiteout, PATH_MAX, "%s%s/.wh.%s",
             state->upper_dir, dir_part, basename);

    /* Create the whiteout marker (empty file) */
    int fd = open(whiteout, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) close(fd);

    return 0;
}

/* ============================================================
 * unionfs_create - Create a new empty file in the upper layer.
 * New files always go to upper — lower is read-only.
 * ============================================================ */
int unionfs_create(const char *path, mode_t mode,
                   struct fuse_file_info *fi) {
    struct mini_unionfs_state *state = UNIONFS_DATA;

    char upper_path[PATH_MAX];
    snprintf(upper_path, PATH_MAX, "%s%s", state->upper_dir, path);

    int fd = open(upper_path, O_CREAT | O_WRONLY | O_TRUNC, mode);
    if (fd < 0) return -errno;
    close(fd);
    return 0;
}

/* ============================================================
 * unionfs_mkdir - Create a new directory in the upper layer.
 * ============================================================ */
int unionfs_mkdir(const char *path, mode_t mode) {
    struct mini_unionfs_state *state = UNIONFS_DATA;

    char upper_path[PATH_MAX];
    snprintf(upper_path, PATH_MAX, "%s%s", state->upper_dir, path);

    if (mkdir(upper_path, mode) < 0) return -errno;
    return 0;
}

/* ============================================================
 * unionfs_rmdir - Remove a directory from the upper layer.
 * ============================================================ */
int unionfs_rmdir(const char *path) {
    struct mini_unionfs_state *state = UNIONFS_DATA;

    char upper_path[PATH_MAX];
    snprintf(upper_path, PATH_MAX, "%s%s", state->upper_dir, path);

    if (rmdir(upper_path) < 0) return -errno;
    return 0;
}

/* ============================================================
 * unionfs_utimens - Update file timestamps.
 * Required for touch, cp, and other POSIX utilities to work.
 * Always updates the upper-layer copy.
 * ============================================================ */
int unionfs_utimens(const char *path, const struct timespec tv[2],
                    struct fuse_file_info *fi) {
    struct mini_unionfs_state *state = UNIONFS_DATA;

    char upper_path[PATH_MAX];
    snprintf(upper_path, PATH_MAX, "%s%s", state->upper_dir, path);

    return utimensat(0, upper_path, tv, 0);
}
