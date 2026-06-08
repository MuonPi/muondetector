#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#include <optional>
#include <string>

class Credentials {
  public:
    struct Mqtt {
        std::string username{};
        std::string password{};
    };
    static void storeCredentials(const std::string& username, const std::string& password);
    static auto retrieveMqttCredentials() -> std::optional<Mqtt>;
    static auto retrieveCredentials() -> std::string;
    static void clearCredentials();

  private:
    Credentials() = delete;
    ~Credentials() = delete;
};

#endif // CREDENTIALS_H