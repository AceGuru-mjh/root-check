#include "../ipc.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

int ipc_server_start(const char *socket_path) {
    unlink(socket_path);
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    chmod(socket_path, 0700);
    if (listen(fd, 5) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

int ipc_server_accept(int server_fd) {
    struct sockaddr_un client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    return client_fd;
}

void ipc_server_stop(int server_fd) {
    if (server_fd >= 0) close(server_fd);
}