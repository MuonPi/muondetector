#ifndef NETWORK_DISCOVERY_H
#define NETWORK_DISCOVERY_H

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <vector>

struct DeviceInfo {
    std::string name;
    std::string ip;
    uint16_t port;
    uint16_t type;
};

class NetworkDiscovery {
  public:
    enum class DeviceType : uint16_t { GUI = 1, DAEMON = 2 };

    using Callback = std::function<void(const DeviceInfo&)>;

    NetworkDiscovery(DeviceType type, uint16_t port = 45454);
    ~NetworkDiscovery();

    void start();
    void stop();

    void discover(); // broadcast request

    void setCallback(Callback cb);

  private:
    void receiverLoop();

  private:
    int sock{-1};
    int control_fd{-1};
    std::atomic<bool> running{false};
    std::thread rxThread;
    uint16_t port;
    DeviceType type;

    Callback callback;
};

#endif // NETWORK_DISCOVERY_H