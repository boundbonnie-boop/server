#include "../server/server.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <iostream>
#include <sys/epoll.h>

server::Logger::ptr g_logger = SERVER_LOG_ROOT();

int sock = 0;

void test_fiber() {
    SERVER_LOG_INFO(g_logger) << "test_fiber sock=" << sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "112.80.248.76", &addr.sin_addr.s_addr);

    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {

    }
    else if(errno == EINPROGRESS) {
        SERVER_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        server::IOManager::GetThis()->addEvent(sock, server::IOManager::READ, [](){
            SERVER_LOG_INFO(g_logger) << "read callback";
            
        });
        server::IOManager::GetThis()->addEvent(sock, server::IOManager::WRITE, [](){
            SERVER_LOG_INFO(g_logger) << "write callback";
            server::IOManager::GetThis()->cancelEvent(sock, server::IOManager::READ);
            close(sock);
        });
    } else {
        SERVER_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
   
}

void test1() {
    SERVER_LOG_INFO(g_logger) << "EPOLLIN=" << EPOLLIN << " EPOLLOUT=" << EPOLLOUT;
    server::IOManager iom;
    iom.schedule(&test_fiber);
}

server::Timer::ptr timer;
void test_timer() {
    server::IOManager iom(2);
    timer = iom.addTimer(1000, []() {
        static int i = 0;
        SERVER_LOG_INFO(g_logger) << "hello timer i = " << i;
        
        if(++i == 5) {
            timer->reset(2000, true);
            // timer->cancel();
        }
        if(i == 8) {
            timer->cancel();
        }
    }, true);
}

int main() {
    // test1();
    test_timer();

    return 0;
}