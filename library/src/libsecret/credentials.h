#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#include <string>

class Credentials {
  public:
    static void storeCredentials(const std::string& username, const std::string& password);
    static auto retrieveCredentials(const std::string& username) -> std::string;

  private:
    Credentials() = delete;
    ~Credentials() = delete;
};

#endif // CREDENTIALS_H