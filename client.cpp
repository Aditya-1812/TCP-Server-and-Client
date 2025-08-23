#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

const size_t k_max_msg = 4096;

static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;
        }
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;
        }
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t send_request(int fd, const std::string &text) {
    uint32_t len = (uint32_t)text.size();
    if (len > k_max_msg) return -1;

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], text.data(), len);

    return write_all(fd, wbuf, 4 + len);
}

static int32_t read_reply(int fd) {
    char rbuf[4 + k_max_msg];
    int32_t err = read_full(fd, rbuf, 4);
    if (err) return err;

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);
    if (len > k_max_msg) return -1;

    err = read_full(fd, &rbuf[4], len);
    if (err) return err;

    std::string reply(rbuf + 4, len);
    std::cout << "Server reply: " << reply << std::endl;
    return 0;
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // connect to localhost

    int rv = connect(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        perror("connect");
        return 1;
    }

    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;

        if (send_request(fd, line)) break;
        if (read_reply(fd)) break;

        if (line == "QUIT") break;
    }

    close(fd);
    return 0;
}
