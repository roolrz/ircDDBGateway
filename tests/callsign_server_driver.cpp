#include "CacheManager.h"
#include "Timer.h"
#include <wx/wx.h>
#include <atomic>
#include <netinet/in.h>
#define private public
#include "CallsignServer.h"
#undef private
#include <arpa/inet.h>
#include <netdb.h>
#include <cstdlib>
#include <iostream>

// Redirect only this isolated test executable to the loopback test server.
extern "C" hostent* __wrap_gethostbyname(const char*) {
    static in_addr address;
    address.s_addr = htonl(INADDR_LOOPBACK);
    static char* addresses[] = {reinterpret_cast<char*>(&address), NULL};
    static hostent host = {};
    host.h_addrtype = AF_INET; host.h_length = sizeof(address); host.h_addr_list = addresses;
    return &host;
}
extern "C" int __real_connect(int, const sockaddr*, socklen_t);
extern "C" int __wrap_connect(int fd, const sockaddr* address, socklen_t length) {
    sockaddr_in target = *reinterpret_cast<const sockaddr_in*>(address);
    target.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    target.sin_port = htons(std::atoi(std::getenv("TEST_PORT")));
    return __real_connect(fd, reinterpret_cast<const sockaddr*>(&target), length);
}
int main(int argc, char** argv) {
    wxInitializer wx;
    if (!wx.IsOk() || argc != 2) return 2;
    CCacheManager cache;
    cache.updateGateway(wxT("DCS999 G"), wxT("127.0.0.9"), DP_DCS, false, true);
    CCallsignServer server(wxT("N0CALL G"), wxT("127.0.0.1"), &cache);
    wxStopWatch elapsed;
    if (std::string(argv[1]) == "startup") {
        server.start();
        if (elapsed.Time() > 500L) return 3;
        wxMilliSleep(200);
        elapsed.Start();
        server.stop();
        if (elapsed.Time() > 2000L) return 4;
    } else {
        server.process(wxT("127.0.0.1"), std::atoi(std::getenv("TEST_PORT")));
        CGatewayData* loaded = cache.findGateway(wxT("DCS123 G"));
        bool expected = std::string(argv[1]) == "complete";
        if ((loaded != NULL) != expected) return 5;
        delete loaded;
    }
    CGatewayData* existing = cache.findGateway(wxT("DCS999 G"));
    if (!existing) return 6;
    delete existing;
    std::cout << "PASS " << argv[1] << " " << elapsed.Time() << "ms\n";
}
