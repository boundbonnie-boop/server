#include "server/hook.h"
#include "server/iomanager.h"
#include "server/log.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>

server::Logger::ptr g_logger = SERVER_LOG_ROOT();

void test_sleep() {
    server::IOManager iom(1);
    iom.schedule([]() {
        sleep(2);
        SERVER_LOG_INFO(g_logger) << "sleep 2";
    });

    iom.schedule([]() {
        sleep(3);
        SERVER_LOG_INFO(g_logger) << "sleep 3";
    });

    SERVER_LOG_INFO(g_logger) << "test_sleep";
}

void test_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "112.80.248.76", &addr.sin_addr.s_addr);

    SERVER_LOG_INFO(g_logger) << "begin connect";
    int ret = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    SERVER_LOG_INFO(g_logger) << "connect ret=" << ret << " errno=" << errno
        << " errstr=" << strerror(errno);
    if(ret) {
        return;
    }

    const char* buf = "GET / HTTP/1.0\r\n\r\n";
    ret = send(sock, buf, strlen(buf), 0);
    SERVER_LOG_INFO(g_logger) << "send ret=" << ret << " errno=" << errno
        << " errstr=" << strerror(errno);
    
    if(ret <= 0) {
        close(sock);
        return;
    }
    std::string buffer;
    buffer.resize(4096);
    ret = recv(sock, &buffer[0], buffer.size(), 0);
    SERVER_LOG_INFO(g_logger) << "recv ret=" << ret << " errno=" << errno
        << " errstr=" << strerror(errno);
    if(ret <= 0) {
        close(sock);
        return;
    }
    buffer.resize(ret);
    SERVER_LOG_INFO(g_logger) << "recv data=" << buffer;
}

int main() {
    // test_sleep();
    server::IOManager iom;
    iom.schedule(test_socket);
    // test_socket();

    return 0;
}