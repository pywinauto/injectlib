#ifndef _WIN32

#include "qt_server.h"

#include <thread>

__attribute__((constructor)) static void qt_srv_attach() {
    std::thread([] {
        QtHelloServer::bootstrap();
    }).detach();
}

__attribute__((destructor)) static void qt_srv_detach() {
    QtHelloServer::shutdown();
}

#endif
