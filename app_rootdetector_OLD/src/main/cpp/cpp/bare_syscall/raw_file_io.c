/*
 * raw_file_io.c - Raw file I/O operations implementation
 *
 * Domain D: 底层裸系统汇编抽象层
 * All operations use bare syscalls, NO libc dependency
 */

#include "raw_file_io.h"
#include "syscall_arm64.h"

__attribute__((visibility("default")))
int raw_open(const char *path, int flags, int mode) {
    long ret = bare_openat(BARE_AT_FDCWD, path, flags, mode);
    return (int)ret;
}

__attribute__((visibility("default")))
long raw_read(int fd, void *buf, size_t count) {
    return bare_read(fd, buf, count);
}

__attribute__((visibility("default")))
long raw_write(int fd, const void *buf, size_t count) {
    return bare_write(fd, buf, count);
}

__attribute__((visibility("default")))
int raw_close(int fd) {
    long ret = bare_close(fd);
    if (bare_is_error(ret)) {
        return (int)ret;
    }
    return 0;
}

__attribute__((visibility("default")))
long raw_seek(int fd, long offset, int whence) {
    return bare_lseek(fd, offset, whence);
}

__attribute__((visibility("default")))
void *raw_mmap(void *addr, size_t length, int prot, int flags, int fd, long offset) {
    long ret = bare_mmap(addr, length, prot, flags, fd, offset);
    if (bare_is_error(ret)) {
        return BARE_MAP_FAILED;
    }
    return (void *)ret;
}

__attribute__((visibility("default")))
int raw_munmap(void *addr, size_t length) {
    long ret = bare_munmap(addr, length);
    if (bare_is_error(ret)) {
        return (int)ret;
    }
    return 0;
}

__attribute__((visibility("default")))
int read_file_raw(const char *path, void **out_buf, size_t *out_size) {
    if (!path || !out_buf || !out_size) {
        return RAW_IO_ERR_INVALID;
    }

    *out_buf = NULL;
    *out_size = 0;

    /* Open file */
    long fd = bare_openat(BARE_AT_FDCWD, path, BARE_O_RDONLY, 0);
    if (bare_is_error(fd)) {
        return RAW_IO_ERR_OPEN;
    }

    /* Get file size via fstat */
    struct kernel_stat st;
    long ret = bare_fstat((int)fd, &st);
    if (bare_is_error(ret)) {
        /* For /proc files, st_size may be 0; read incrementally */
        size_t buf_size = 4096;
        void *buf = bare_mmap_ptr(buf_size);
        if (!buf) {
            bare_close((int)fd);
            return RAW_IO_ERR_MMAP;
        }

        size_t total = 0;
        while (1) {
            if (total + 4096 > buf_size) {
                size_t new_size = buf_size * 2;
                if (new_size > MAX_RAW_FILE_SIZE) {
                    raw_munmap(buf, buf_size);
                    bare_close((int)fd);
                    return RAW_IO_ERR_TOO_LARGE;
                }
                void *new_buf = bare_mmap_ptr(new_size);
                if (!new_buf) {
                    raw_munmap(buf, buf_size);
                    bare_close((int)fd);
                    return RAW_IO_ERR_MMAP;
                }
                /* Copy old data */
                for (size_t i = 0; i < total; i++) {
                    ((unsigned char *)new_buf)[i] = ((unsigned char *)buf)[i];
                }
                raw_munmap(buf, buf_size);
                buf = new_buf;
                buf_size = new_size;
            }

            ret = bare_read((int)fd, (char *)buf + total, buf_size - total);
            if (bare_is_error(ret)) {
                raw_munmap(buf, buf_size);
                bare_close((int)fd);
                return RAW_IO_ERR_READ;
            }
            if (ret == 0) break;
            total += (size_t)ret;
        }

        bare_close((int)fd);
        *out_buf = buf;
        *out_size = total;
        return RAW_IO_OK;
    }

    int64_t file_size = st.st_size;
    if (file_size < 0 || (uint64_t)file_size > MAX_RAW_FILE_SIZE) {
        bare_close((int)fd);
        return file_size > MAX_RAW_FILE_SIZE ? RAW_IO_ERR_TOO_LARGE : RAW_IO_ERR_READ;
    }

    if (file_size == 0) {
        /* /proc or /sys file: read incrementally */
        bare_close((int)fd);

        fd = bare_openat(BARE_AT_FDCWD, path, BARE_O_RDONLY, 0);
        if (bare_is_error(fd)) {
            return RAW_IO_ERR_OPEN;
        }

        size_t buf_size = 4096;
        void *buf = bare_mmap_ptr(buf_size);
        if (!buf) {
            bare_close((int)fd);
            return RAW_IO_ERR_MMAP;
        }

        size_t total = 0;
        while (1) {
            if (total + 4096 > buf_size) {
                size_t new_size = buf_size * 2;
                if (new_size > MAX_RAW_FILE_SIZE) {
                    raw_munmap(buf, buf_size);
                    bare_close((int)fd);
                    return RAW_IO_ERR_TOO_LARGE;
                }
                void *new_buf = bare_mmap_ptr(new_size);
                if (!new_buf) {
                    raw_munmap(buf, buf_size);
                    bare_close((int)fd);
                    return RAW_IO_ERR_MMAP;
                }
                for (size_t i = 0; i < total; i++) {
                    ((unsigned char *)new_buf)[i] = ((unsigned char *)buf)[i];
                }
                raw_munmap(buf, buf_size);
                buf = new_buf;
                buf_size = new_size;
            }

            ret = bare_read((int)fd, (char *)buf + total, buf_size - total);
            if (bare_is_error(ret)) {
                raw_munmap(buf, buf_size);
                bare_close((int)fd);
                return RAW_IO_ERR_READ;
            }
            if (ret == 0) break;
            total += (size_t)ret;
        }

        bare_close((int)fd);
        *out_buf = buf;
        *out_size = total;
        return RAW_IO_OK;
    }

    /* Regular file with known size */
    size_t alloc_size = (size_t)file_size;
    void *buf = bare_mmap_ptr(alloc_size);
    if (!buf) {
        bare_close((int)fd);
        return RAW_IO_ERR_MMAP;
    }

    size_t total_read = 0;
    while (total_read < alloc_size) {
        ret = bare_read((int)fd, (char *)buf + total_read, alloc_size - total_read);
        if (bare_is_error(ret)) {
            raw_munmap(buf, alloc_size);
            bare_close((int)fd);
            return RAW_IO_ERR_READ;
        }
        if (ret == 0) break;
        total_read += (size_t)ret;
    }

    bare_close((int)fd);
    *out_buf = buf;
    *out_size = total_read;
    return RAW_IO_OK;
}

/* Internal helper: allocate anonymous mmap region */
static void *bare_mmap_ptr(size_t size) {
    long ret = bare_mmap(NULL, size, BARE_PROT_READ | BARE_PROT_WRITE,
                         BARE_MAP_PRIVATE | BARE_MAP_ANONYMOUS, -1, 0);
    if (bare_is_error(ret)) {
        return NULL;
    }
    return (void *)ret;
}

__attribute__((visibility("default")))
void raw_free_file(void *buf, size_t size) {
    if (buf && size > 0) {
        raw_munmap(buf, size);
    }
}

__attribute__((visibility("default")))
int read_proc_node(const char *node_path, char **out_buf, size_t *out_size) {
    if (!node_path || !out_buf || !out_size) {
        return RAW_IO_ERR_INVALID;
    }

    *out_buf = NULL;
    *out_size = 0;

    void *buf = NULL;
    size_t size = 0;
    int ret = read_file_raw(node_path, &buf, &size);
    if (ret != RAW_IO_OK) {
        return ret;
    }

    /* Ensure null-terminated */
    if (size == 0 || ((char *)buf)[size - 1] != '\0') {
        void *new_buf = bare_mmap_ptr(size + 1);
        if (!new_buf) {
            raw_munmap(buf, size);
            return RAW_IO_ERR_MMAP;
        }
        for (size_t i = 0; i < size; i++) {
            ((char *)new_buf)[i] = ((char *)buf)[i];
        }
        ((char *)new_buf)[size] = '\0';
        raw_munmap(buf, size);
        buf = new_buf;
        size++;
    }

    *out_buf = (char *)buf;
    *out_size = size;
    return RAW_IO_OK;
}

__attribute__((visibility("default")))
int read_dev_node(const char *dev_path, void **out_buf, size_t *out_size, size_t max_size) {
    if (!dev_path || !out_buf || !out_size) {
        return RAW_IO_ERR_INVALID;
    }

    *out_buf = NULL;
    *out_size = 0;

    if (max_size > MAX_RAW_FILE_SIZE) {
        max_size = MAX_RAW_FILE_SIZE;
    }

    long fd = bare_openat(BARE_AT_FDCWD, dev_path, BARE_O_RDONLY, 0);
    if (bare_is_error(fd)) {
        return RAW_IO_ERR_OPEN;
    }

    void *buf = bare_mmap_ptr(max_size);
    if (!buf) {
        bare_close((int)fd);
        return RAW_IO_ERR_MMAP;
    }

    size_t total_read = 0;
    while (total_read < max_size) {
        long ret = bare_read((int)fd, (char *)buf + total_read, max_size - total_read);
        if (bare_is_error(ret)) {
            raw_munmap(buf, max_size);
            bare_close((int)fd);
            return RAW_IO_ERR_READ;
        }
        if (ret == 0) break;
        total_read += (size_t)ret;
    }

    bare_close((int)fd);
    *out_buf = buf;
    *out_size = total_read;
    return RAW_IO_OK;
}

__attribute__((visibility("default")))
int raw_stat(const char *path, raw_file_info_t *info) {
    if (!path || !info) {
        return RAW_IO_ERR_INVALID;
    }

    struct kernel_stat kst;
    long ret = bare_stat(path, &kst);
    if (bare_is_error(ret)) {
        return RAW_IO_ERR_NOT_FOUND;
    }

    info->size = kst.st_size;
    info->mode = kst.st_mode;
    info->uid = kst.st_uid;
    info->gid = kst.st_gid;
    info->atime = kst.st_atime;
    info->mtime = kst.st_mtime;
    info->ctime = kst.st_ctime;

    return RAW_IO_OK;
}

__attribute__((visibility("default")))
int raw_file_exists(const char *path) {
    if (!path) return 0;
    long ret = bare_access(path, BARE_F_OK);
    return bare_is_error(ret) ? 0 : 1;
}

__attribute__((visibility("default")))
char *raw_read_string(const char *path) {
    if (!path) return NULL;

    void *buf = NULL;
    size_t size = 0;

    int ret = read_file_raw(path, &buf, &size);
    if (ret != RAW_IO_OK) return NULL;

    /* Ensure null-terminated */
    if (size == 0 || ((char *)buf)[size - 1] != '\0') {
        void *new_buf = bare_mmap_ptr(size + 1);
        if (!new_buf) {
            raw_munmap(buf, size);
            return NULL;
        }
        for (size_t i = 0; i < size; i++) {
            ((char *)new_buf)[i] = ((char *)buf)[i];
        }
        ((char *)new_buf)[size] = '\0';
        raw_munmap(buf, size);
        buf = new_buf;
    }

    return (char *)buf;
}

__attribute__((visibility("default")))
long raw_readlink(const char *path, char *buf, size_t bufsize) {
    if (!path || !buf || bufsize == 0) {
        return -BARE_EINVAL;
    }

    long ret = bare_readlinkat(BARE_AT_FDCWD, path, buf, bufsize);
    if (bare_is_error(ret)) {
        return ret;
    }

    if ((size_t)ret < bufsize) {
        buf[ret] = '\0';
    }

    return ret;
}

__attribute__((visibility("default")))
int raw_listdir(const char *dir_path, raw_dirent_callback callback, void *user_data) {
    if (!dir_path || !callback) {
        return -BARE_EINVAL;
    }

    long fd = bare_openat(BARE_AT_FDCWD, dir_path, BARE_O_RDONLY | BARE_O_DIRECTORY, 0);
    if (bare_is_error(fd)) {
        return (int)fd;
    }

    size_t buf_size = 4096;
    void *buf = bare_mmap_ptr(buf_size);
    if (!buf) {
        bare_close((int)fd);
        return -BARE_ENOMEM;
    }

    int entry_count = 0;

    while (1) {
        long nread = bare_getdents64((int)fd, buf, buf_size);
        if (bare_is_error(nread)) {
            raw_munmap(buf, buf_size);
            bare_close((int)fd);
            return (int)nread;
        }
        if (nread == 0) break;

        size_t pos = 0;
        while (pos < (size_t)nread) {
            struct linux_dirent64 *entry = (struct linux_dirent64 *)((char *)buf + pos);

            /* Skip . and .. */
            if (entry->d_name[0] == '.' &&
                (entry->d_name[1] == '\0' ||
                 (entry->d_name[1] == '.' && entry->d_name[2] == '\0'))) {
                pos += entry->d_reclen;
                continue;
            }

            int result = callback(entry->d_name, entry->d_type, user_data);
            if (result != 0) {
                raw_munmap(buf, buf_size);
                bare_close((int)fd);
                return entry_count;
            }

            entry_count++;
            pos += entry->d_reclen;
        }
    }

    raw_munmap(buf, buf_size);
    bare_close((int)fd);
    return entry_count;
}
