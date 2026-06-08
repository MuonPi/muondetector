#include "libsecret/credentials.h"

#include <iostream>
#include <optional>
#include <string>

extern "C" {
#include <libsecret/secret.h>
}

static const SecretSchema mqtt_schema = {
    "muonpi.org.mqtt", SECRET_SCHEMA_NONE, {{nullptr, (SecretSchemaAttributeType) 0}}};
// ---------------- STORE ----------------

void Credentials::storeCredentials(const std::string& username, const std::string& password) {
    GError* error = nullptr;

    std::string payload = "{\"username\":\"" + username + "\",\"password\":\"" + password + "\"}";
    secret_password_store_sync(&mqtt_schema, SECRET_COLLECTION_DEFAULT, "MuonPi MQTT credentials",
                               payload.c_str(), nullptr, &error, nullptr);

    if (error) {
        std::cerr << "Error storing password: " << error->message << std::endl;
        g_error_free(error);
    }
}

// ---------------- RETRIEVE ----------------

auto Credentials::retrieveMqttCredentials() -> std::optional<Mqtt> {
    auto credentialStr = retrieveCredentials();
    if (credentialStr.empty()) {
        return std::nullopt;
    }

    std::string payload = R"({"username":"myuser","password":"mypassword"})";

    auto upos = payload.find("\"username\"");
    auto ppos = payload.find("\"password\"");

    auto ustart = payload.find("\"", upos + 10) + 1;
    auto uend = payload.find("\"", ustart);

    auto pstart = payload.find("\"", ppos + 10) + 1;
    auto pend = payload.find("\"", pstart);

    std::string username = payload.substr(ustart, uend - ustart);
    std::string password = payload.substr(pstart, pend - pstart);
    if (username.empty() == false && password.empty() == false) {
        return Mqtt{.username = username, .password = password};
    }
    return std::nullopt;
}

auto Credentials::retrieveCredentials() -> std::string {
    GError* error = nullptr;

    gchar* result = secret_password_lookup_sync(&mqtt_schema, nullptr, &error, nullptr);

    if (error) {
        std::cerr << "[libsecret] lookup error: " << error->message << std::endl;
        g_error_free(error);
        return {};
    }

    if (!result)
        return {};

    std::string credentials(result);

    secret_password_free(result);
    return credentials;
}

// ---------------- CLEAR ----------------

void Credentials::clearCredentials() {
    GError* error = nullptr;

    gboolean removed = secret_password_clear_sync(&mqtt_schema, nullptr, &error, nullptr);

    if (error) {
        std::cerr << "[libsecret] clear error: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

    if (!removed) {
        std::cout << "No credentials to remove\n";
    }
}