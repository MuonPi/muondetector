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

    NetworkDiscovery(DeviceType type, uint16_t port);
    ~NetworkDiscovery();

    void start();
    void stop();

    void discover(); // broadcast request

    void setCallback(Callback cb);

  private:
    void receiverLoop();
    void sendBroadcast(const std::string& msg);
    void sendResponse(const std::string& targetIp, const std::string& msg);

  private:
    int sock;
    uint16_t port;
    DeviceType type;

    std::thread rxThread;
    std::atomic<bool> running{false};

    Callback callback;
};

#endif // NETWORK_DISCOVERY_H