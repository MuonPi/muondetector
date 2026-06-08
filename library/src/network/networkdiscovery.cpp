#include "network/networkdiscovery.h"

#ifdef _WIN32
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

using socklen_t = int;
static bool wsa_initialized = false;

static void init_wsa() {
    if (!wsa_initialized) {
        WSADATA data;
        WSAStartup(MAKEWORD(2, 2), &data);
        wsa_initialized = true;
    }
}

static void cleanup_wsa() {
    if (wsa_initialized) {
        WSACleanup();
        wsa_initialized = false;
    }
}

#define CLOSESOCKET closesocket
#define SHUT_RDWR SD_BOTH

#else
#include <arpa/inet.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#define CLOSESOCKET close
#endif

#include <cstring>
#include <iostream>

static constexpr uint32_t MAGIC = 0x2A2A2A2A;
static constexpr uint8_t VERSION = 1;

struct Packet {
    uint32_t magic;
    uint8_t version;
    uint16_t type;
    uint32_t requestId;
};

NetworkDiscovery::NetworkDiscovery(DeviceType t, uint16_t p) : port(p), type(t) {
#ifdef _WIN32
    init_wsa();
#endif

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket\n";
        return;
    }

#ifndef _WIN32
    control_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (control_fd < 0) {
        std::cerr << "Failed to create eventfd\n";
    }
#endif

    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&yes), sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
}

NetworkDiscovery::~NetworkDiscovery() {
    stop();

    if (sock >= 0) {
        shutdown(sock, SHUT_RDWR);
        CLOSESOCKET(sock);
    }

#ifndef _WIN32
    if (control_fd >= 0) {
        close(control_fd);
        control_fd = -1;
    }
#endif

#ifdef _WIN32
    cleanup_wsa();
#endif
}

void NetworkDiscovery::start() {
    running = true;
    rxThread = std::thread(&NetworkDiscovery::receiverLoop, this);
}

void NetworkDiscovery::stop() {
    running = false;

#ifndef _WIN32
    if (control_fd >= 0) {
        uint64_t v = 1;
        write(control_fd, &v, sizeof(v)); // wake select()
    }
#endif

    if (sock >= 0) {
#ifdef _WIN32
        shutdown(sock, SD_BOTH);
#endif
        CLOSESOCKET(sock);
        sock = -1;
    }

    if (rxThread.joinable())
        rxThread.join();
}

void NetworkDiscovery::discover() {
    Packet p{MAGIC, VERSION, static_cast<uint16_t>(type), 12345};

    sockaddr_in bc{};
    bc.sin_family = AF_INET;
    bc.sin_port = htons(port);
    bc.sin_addr.s_addr = INADDR_BROADCAST;

    sendto(sock, reinterpret_cast<const char*>(&p), sizeof(p), 0, reinterpret_cast<sockaddr*>(&bc),
           sizeof(bc));
}

void NetworkDiscovery::receiverLoop() {
    while (running) {
        fd_set readfds;
        FD_ZERO(&readfds);

        FD_SET(sock, &readfds);

#ifndef _WIN32
        FD_SET(control_fd, &readfds);
#endif

        int maxfd = sock;
#ifndef _WIN32
        if (control_fd > maxfd)
            maxfd = control_fd;
#endif

        timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int ret = select(maxfd + 1, &readfds, nullptr, nullptr, &tv);

        if (!running)
            break;

        if (ret <= 0)
            continue;

#ifndef _WIN32
        if (FD_ISSET(control_fd, &readfds)) {
            uint64_t v;
            read(control_fd, &v, sizeof(v));
            break;
        }
#endif

        if (!FD_ISSET(sock, &readfds))
            continue;

        Packet p{};
        sockaddr_in sender{};
        socklen_t len = sizeof(sender);

        int r = recvfrom(sock, reinterpret_cast<char*>(&p), sizeof(p), 0,
                         reinterpret_cast<sockaddr*>(&sender), &len);

        if (r <= 0)
            continue;

        if (r != sizeof(Packet))
            continue;

        if (p.magic != MAGIC || p.version != VERSION)
            continue;

        DeviceType senderType = static_cast<DeviceType>(p.type);

        if (type == DeviceType::DAEMON && senderType == DeviceType::GUI) {
            Packet response{MAGIC, VERSION, static_cast<uint16_t>(DeviceType::DAEMON), p.requestId};

            sendto(sock, reinterpret_cast<const char*>(&response), sizeof(response), 0,
                   reinterpret_cast<sockaddr*>(&sender), sizeof(sender));
        }

        if (type == DeviceType::GUI && senderType == DeviceType::DAEMON) {
            char ip[INET_ADDRSTRLEN];

            inet_ntop(AF_INET, &sender.sin_addr, ip, sizeof(ip));

            if (callback) {
                DeviceInfo info;
                info.ip = ip;
                info.port = ntohs(sender.sin_port);
                info.type = p.type;
                info.name = "daemon";
                callback(info);
            }
        }
    }
}
void NetworkDiscovery::setCallback(Callback cb) {
    callback = std::move(cb);
}