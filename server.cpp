#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>

const size_t k_max_msg = 4096;

static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;
        }
        assert((size_t)rv <= n);
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
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t send_reply(int fd, const std::string &reply) {
    uint32_t len = (uint32_t)reply.size();
    if (len > k_max_msg) return -1;

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply.data(), len);

    return write_all(fd, wbuf, 4 + len);
}

static void handle_client(int connfd) {
    while (true) {
        char rbuf[4 + k_max_msg];
        int32_t err = read_full(connfd, rbuf, 4);
        if (err) {
            msg("Client disconnected");
            break;
        }

        uint32_t len = 0;
        memcpy(&len, rbuf, 4);
        if (len > k_max_msg) {
            msg("Message too long");
            break;
        }

        err = read_full(connfd, &rbuf[4], len);
        if (err) {
            msg("Read error");
            break;
        }

        std::string cmd(rbuf + 4, len);
        fprintf(stderr, "Client says: %s\n", cmd.c_str());

        std::string reply;

        if (cmd.rfind("ECHO ", 0) == 0) {
            reply = cmd.substr(5);
        } else if (cmd == "TIME") {
            auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            reply = std::string(std::ctime(&now));
        } else if (cmd.rfind("UPPER ", 0) == 0) {
            reply = cmd.substr(6);
            std::transform(reply.begin(), reply.end(), reply.begin(), ::toupper);
        } else if (cmd == "QUIT") {
            reply = "Goodbye!";
            send_reply(connfd, reply);
            break;
        } else {
            reply = "Unknown command";
        }

        if (send_reply(connfd, reply)) {
            msg("Write error");
            break;
        }
    }
    close(connfd);
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) die("socket()");

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) die("bind()");

    rv = listen(fd, SOMAXCONN);
    if (rv) die("listen()");

    msg("Server is running on port 1234...");

    while (true) {
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
        if (connfd < 0) continue;

        std::thread t(handle_client, connfd);
        t.detach();
    }
    return 0;
}
