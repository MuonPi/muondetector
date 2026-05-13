#include "network/networkdiscovery.h"

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <unistd.h>

static constexpr uint32_t MAGIC = 0x2A2A2A2A;
static constexpr uint8_t VERSION = 1;

struct Packet {
    uint32_t magic;
    uint8_t version;
    uint16_t type;
    uint32_t requestId;
};

NetworkDiscovery::NetworkDiscovery(DeviceType t, uint16_t p) : port(p), type(t) {

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (sockaddr*) &addr, sizeof(addr));
}

NetworkDiscovery::~NetworkDiscovery() {
    stop();
    close(sock);
}

void NetworkDiscovery::start() {
    running = true;
    rxThread = std::thread(&NetworkDiscovery::receiverLoop, this);
}

void NetworkDiscovery::stop() {
    running = false;
    if (rxThread.joinable())
        rxThread.join();
}

void NetworkDiscovery::discover() {
    Packet p{MAGIC, VERSION, static_cast<uint16_t>(type), 12345};

    sockaddr_in bc{};
    bc.sin_family = AF_INET;
    bc.sin_port = htons(port);
    bc.sin_addr.s_addr = INADDR_BROADCAST;

    sendto(sock, &p, sizeof(p), 0, (sockaddr*) &bc, sizeof(bc));
}

void NetworkDiscovery::receiverLoop() {
    while (running) {
        Packet p{};
        sockaddr_in sender{};
        socklen_t len = sizeof(sender);

        int r = recvfrom(sock, &p, sizeof(p), 0, (sockaddr*) &sender, &len);

        if (r <= 0)
            continue;

        if (p.magic != MAGIC)
            continue;
        if (p.version != VERSION)
            continue;

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sender.sin_addr, ip, sizeof(ip));

        if (p.type == static_cast<uint16_t>(type)) {
            continue; // ignore self-type echo (simple filter)
        }

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

void NetworkDiscovery::setCallback(Callback cb) {
    callback = std::move(cb);
}