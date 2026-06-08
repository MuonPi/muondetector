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

NetworkDiscovery::NetworkDiscovery(DeviceType t, uint16_t p)
    : port(p), type(t)
{
#ifdef _WIN32
    init_wsa();
#endif

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket\n";
        return;
    }

    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
               reinterpret_cast<const char*>(&yes),
               sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
}

NetworkDiscovery::~NetworkDiscovery()
{
    stop();

    if (sock >= 0) {
        shutdown(sock, SHUT_RDWR);
        CLOSESOCKET(sock);
    }

#ifdef _WIN32
    cleanup_wsa();
#endif
}

void NetworkDiscovery::start()
{
    running = true;
    rxThread = std::thread(&NetworkDiscovery::receiverLoop, this);
}

void NetworkDiscovery::stop()
{
    running = false;

    if (rxThread.joinable())
        rxThread.join();
}

void NetworkDiscovery::discover()
{
    Packet p{MAGIC, VERSION,
             static_cast<uint16_t>(type),
             12345};

    sockaddr_in bc{};
    bc.sin_family = AF_INET;
    bc.sin_port = htons(port);
    bc.sin_addr.s_addr = INADDR_BROADCAST;

    sendto(sock,
           reinterpret_cast<const char*>(&p),
           sizeof(p),
           0,
           reinterpret_cast<sockaddr*>(&bc),
           sizeof(bc));
}

void NetworkDiscovery::receiverLoop()
{
    while (running) {

        Packet p{};
        sockaddr_in sender{};
        socklen_t len = sizeof(sender);

        int r = recvfrom(sock,
                         reinterpret_cast<char*>(&p),
                         sizeof(p),
                         0,
                         reinterpret_cast<sockaddr*>(&sender),
                         &len);

        if (!running)
            break;

        if (r <= 0)
            continue;

        if (r != sizeof(Packet))
            continue;

        if (p.magic != MAGIC)
            continue;

        if (p.version != VERSION)
            continue;

        DeviceType senderType = static_cast<DeviceType>(p.type);

        // GUI asking for daemons
        if (type == DeviceType::DAEMON &&
            senderType == DeviceType::GUI)
        {
            Packet response{MAGIC, VERSION,
                            static_cast<uint16_t>(DeviceType::DAEMON),
                            p.requestId};

            sendto(sock,
                   reinterpret_cast<const char*>(&response),
                   sizeof(response),
                   0,
                   reinterpret_cast<sockaddr*>(&sender),
                   sizeof(sender));

            continue;
        }

        // GUI receives daemon response
        if (type == DeviceType::GUI &&
            senderType == DeviceType::DAEMON)
        {
            char ip[INET_ADDRSTRLEN];

            inet_ntop(AF_INET,
                       &sender.sin_addr,
                       ip,
                       sizeof(ip));

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

void NetworkDiscovery::setCallback(Callback cb)
{
    callback = std::move(cb);
}