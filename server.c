#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

static const char* const host = "127.0.0.1";
static const short port = 7777;

void io_handler(int sig, siginfo_t *info, void *ucontext);

int main() {
    struct sigaction act;
    memset(&act, sizeof(act), 0);
    sigemptyset(&act.sa_mask);
    act.sa_sigaction = io_handler;
    act.sa_flags = SA_SIGINFO;

    int result = sigaction(SIGIO, &act, NULL);
    if (result == -1) {
        fprintf(stderr, "[ERROR] sigaction error\n");
        return -1;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        fprintf(stderr, "[ERROR] socket error\n");
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);

    socklen_t addrLength = sizeof(addr);

    result = bind(fd, (struct sockaddr*)(&addr), addrLength);
    if (result == -1) {
        fprintf(stderr, "[ERROR] bind error\n");
        return -1;
    }

    int backlog = 10;
    result = listen(fd, backlog);
    if (result == -1) {
        fprintf(stderr, "[ERROR] listen error\n");
        return -1;
    }

    while (1) {
        int clientFd = accept(fd, (struct sockaddr*)(&addr), &addrLength);
        if (clientFd == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                fprintf(stderr, "[ERROR] accept error\n");
                break;
            }
        }

        fprintf(stdout, "[INFO] accept client, fd: %d, port: %d\n", clientFd, (int)(htons(addr.sin_port)));

        result = fcntl(clientFd, F_SETOWN, getpid());
        if (result == -1) {
            fprintf(stderr, "[ERROR] fcntl F_SETOWN\n");
            break;
        }

        int flags = fcntl(clientFd, F_GETFL);
        flags |= O_ASYNC | O_NONBLOCK;
        result = fcntl(clientFd, F_SETFL, flags);
        if (result == -1) {
            fprintf(stderr, "[ERROR] fcntl F_SETFL\n");
            break;
        }

        result = fcntl(clientFd, F_SETSIG, SIGIO);
        if (result == -1) {
            fprintf(stderr, "[ERROR] fcntl F_SETSIG error\n");
            break;
        }
    }

    close(fd);
    return 0;
}

void io_handler(int sig, siginfo_t *info, void *ucontext) {
    int fd = info->si_fd;
    long flag = info->si_band;
    if ((flag & POLLIN) == 0) {
        return;
    }

    while (1) {
        static const int kPrefixSize = 6;
        static const int kBufferSize = 1024;
        char buf[kPrefixSize + kBufferSize];
        strcpy(buf, "Hello:");
        ssize_t bytesReceived = recv(fd, buf + kPrefixSize, kBufferSize, 0);
        if (bytesReceived == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            fprintf(stderr, "[ERROR] recv error\n");
            close(fd);
            break;
        }

        if (bytesReceived == 0) {
            fprintf(stdout, "[INFO] connection closed by peer, fd: %d\n", fd);
            close(fd);
            break;
        }

        fprintf(stdout, "[INFO] %d bytes received by fd %d\n", bytesReceived, fd);

        int bytesToSent = kPrefixSize + bytesReceived;
        ssize_t bytesSent = send(fd, buf, bytesToSent, 0);
        if (bytesSent != bytesToSent) {
            fprintf(stderr, "[ERROR] send error\n");
        }
    }
}
