/*
 * raw_file_io.h - Raw file I/O operations via bare syscalls
 *
 * Domain D: 底层裸系统汇编抽象层
 * All operations use bare syscalls, NO libc dependency
 */

#ifndef RAW_FILE_IO_H
#define RAW_FILE_IO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum file size for raw operations (16 MB) */
#define MAX_RAW_FILE_SIZE   (16 * 1024 * 1024)

/* Default buffer size for file reading */
#define RAW_IO_BUF_SIZE     (4 * 1024)

/* Error codes */
#define RAW_IO_OK               0
#define RAW_IO_ERR_OPEN         (-1)
#define RAW_IO_ERR_READ         (-2)
#define RAW_IO_ERR_WRITE        (-3)
#define RAW_IO_ERR_MMAP         (-4)
#define RAW_IO_ERR_CLOSE        (-5)
#define RAW_IO_ERR_SEEK         (-6)
#define RAW_IO_ERR_TOO_LARGE    (-7)
#define RAW_IO_ERR_NOT_FOUND    (-8)
#define RAW_IO_ERR_INVALID      (-9)

/* File info structure */
typedef struct {
    int64_t     size;
    uint32_t    mode;
    uint32_t    uid;
    uint32_t    gid;
    int64_t     atime;
    int64_t     mtime;
    int64_t     ctime;
} raw_file_info_t;

/**
 * Open a file using bare syscall
 *
 * @param path   File path
 * @param flags  Open flags (BARE_O_RDONLY, BARE_O_WRONLY, etc.)
 * @param mode   File mode (for creation)
 * @return File descriptor on success, negative errno on failure
 */
__attribute__((visibility("default")))
int raw_open(const char *path, int flags, int mode);

/**
 * Read from file descriptor using bare syscall
 *
 * @param fd     File descriptor
 * @param buf    Buffer to read into
 * @param count  Number of bytes to read
 * @return Bytes read on success, negative errno on failure
 */
__attribute__((visibility("default")))
long raw_read(int fd, void *buf, size_t count);

/**
 * Write to file descriptor using bare syscall
 *
 * @param fd     File descriptor
 * @param buf    Buffer to write from
 * @param count  Number of bytes to write
 * @return Bytes written on success, negative errno on failure
 */
__attribute__((visibility("default")))
long raw_write(int fd, const void *buf, size_t count);

/**
 * Close file descriptor using bare syscall
 *
 * @param fd  File descriptor
 * @return 0 on success, negative errno on failure
 */
__attribute__((visibility("default")))
int raw_close(int fd);

/**
 * Seek in file using bare syscall
 *
 * @param fd      File descriptor
 * @param offset  Offset value
 * @param whence  Seek mode (BARE_SEEK_SET, BARE_SEEK_CUR, BARE_SEEK_END)
 * @return New offset on success, negative errno on failure
 */
__attribute__((visibility("default")))
long raw_seek(int fd, long offset, int whence);

/**
 * Memory map a file using bare syscall
 *
 * @param addr    Hint address (NULL for auto)
 * @param length  Mapping length
 * @param prot    Protection flags (BARE_PROT_READ, etc.)
 * @param flags   Mapping flags (BARE_MAP_SHARED, etc.)
 * @param fd      File descriptor
 * @param offset  File offset
 * @return Mapped address on success, BARE_MAP_FAILED on failure
 */
__attribute__((visibility("default")))
void *raw_mmap(void *addr, size_t length, int prot, int flags, int fd, long offset);

/**
 * Unmap memory region using bare syscall
 *
 * @param addr    Mapped address
 * @param length  Mapping length
 * @return 0 on success, negative errno on failure
 */
__attribute__((visibility("default")))
int raw_munmap(void *addr, size_t length);

/**
 * Read entire file into buffer (allocates via mmap)
 *
 * @param path      File path
 * @param out_buf   Output buffer pointer (caller must free with raw_free_file)
 * @param out_size  Output file size
 * @return RAW_IO_OK on success, negative error code on failure
 */
__attribute__((visibility("default")))
int read_file_raw(const char *path, void **out_buf, size_t *out_size);

/**
 * Free buffer allocated by read_file_raw
 *
 * @param buf   Buffer to free
 * @param size  Buffer size
 */
__attribute__((visibility("default")))
void raw_free_file(void *buf, size_t size);

/**
 * Read /proc node content
 *
 * @param node_path  Path to /proc node (e.g., "/proc/cpuinfo")
 * @param out_buf    Output buffer (caller must free)
 * @param out_size   Output size
 * @return RAW_IO_OK on success, negative error code on failure
 */
__attribute__((visibility("default")))
int read_proc_node(const char *node_path, char **out_buf, size_t *out_size);

/**
 * Read /dev node content (binary safe)
 *
 * @param dev_path   Path to /dev node
 * @param out_buf    Output buffer (caller must free)
 * @param out_size   Output size
 * @param max_size   Maximum bytes to read
 * @return RAW_IO_OK on success, negative error code on failure
 */
__attribute__((visibility("default")))
int read_dev_node(const char *dev_path, void **out_buf, size_t *out_size, size_t max_size);

/**
 * Get file information using bare syscall
 *
 * @param path  File path
 * @param info  Output file info structure
 * @return RAW_IO_OK on success, negative error code on failure
 */
__attribute__((visibility("default")))
int raw_stat(const char *path, raw_file_info_t *info);

/**
 * Check if file exists
 *
 * @param path  File path
 * @return 1 if exists, 0 if not, negative errno on error
 */
__attribute__((visibility("default")))
int raw_file_exists(const char *path);

/**
 * Read file content as string (null-terminated)
 *
 * @param path  File path
 * @return Allocated string (caller must free), NULL on failure
 */
__attribute__((visibility("default")))
char *raw_read_string(const char *path);

/**
 * Read symlink target
 *
 * @param path      Symlink path
 * @param buf       Output buffer
 * @param bufsize   Buffer size
 * @return Length of target on success, negative errno on failure
 */
__attribute__((visibility("default")))
long raw_readlink(const char *path, char *buf, size_t bufsize);

/**
 * List directory entries using getdents64
 *
 * @param dir_path  Directory path
 * @param callback  Callback function for each entry
 * @param user_data User data passed to callback
 * @return Number of entries on success, negative errno on failure
 */
typedef int (*raw_dirent_callback)(const char *name, unsigned char type, void *user_data);

__attribute__((visibility("default")))
int raw_listdir(const char *dir_path, raw_dirent_callback callback, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* RAW_FILE_IO_H */
