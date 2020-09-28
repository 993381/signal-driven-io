#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* const host = "127.0.0.1";
static const short port = 7777;

int main() {
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
    int result = connect(fd, (struct sockaddr*)&addr, addrLength);
    if (result == -1) {
        fprintf(stderr, "[ERROR] connect error\n");
        return -1;
    }

    result = getsockname(fd, (struct sockaddr*)&addr, &addrLength);
    if (result == -1) {
        fprintf(stderr, "[ERROR] getsockname error\n");
        return -1;
    }

    printf("local port is: %d\n", (int)(htons(addr.sin_port)));

    while (1) {
        static const int kBufferSize = 1024;
        char buf[kBufferSize];
        fprintf(stdout, ">>");
        scanf("%s", buf);
        if (strcmp(buf, "quit") == 0) {
            break;
        }

        int bytesToSent = (int)(strlen(buf)) + 1;
        ssize_t bytesSent = send(fd, buf, bytesToSent, 0);
        if (bytesSent != bytesToSent) {
            fprintf(stderr, "[ERROR] send error\n");
            break;
        }

        ssize_t bytesReceived = recv(fd, buf, kBufferSize, 0);
        if (bytesReceived == -1) {
            fprintf(stderr, "[ERROR] recv error\n");
            break;
        }

        if (bytesReceived == 0) {
            fprintf(stdout, "[INFO] connection closed by peer\n");
            break;
        }

        fprintf(stdout, buf);
        fprintf(stdout, "\n");
    }

    close(fd);
    return 0;
}
