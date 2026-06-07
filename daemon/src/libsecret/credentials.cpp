#include "libsecret/credentials.h"

#include <iostream>
#include <string>

extern "C" {
#include <libsecret/secret.h>
}

static const SecretSchema mqtt_password_schema = {"muonpi.org.password",
                                                  SECRET_SCHEMA_NONE,
                                                  {{"username", SECRET_SCHEMA_ATTRIBUTE_STRING},
                                                   {"service", SECRET_SCHEMA_ATTRIBUTE_STRING},
                                                   {nullptr, (SecretSchemaAttributeType) 0}}};

// ---------------- STORE ----------------

void storePassword(const std::string& username, const std::string& password) {
    GError* error = nullptr;

    secret_password_store_sync(&mqtt_password_schema, SECRET_COLLECTION_DEFAULT,
                               "MuonPi MQTT password", password.c_str(), nullptr, &error,
                               "username", username.c_str(), nullptr);

    if (error) {
        std::cerr << "Error storing password: " << error->message << std::endl;
        g_error_free(error);
    }
}

// ---------------- RETRIEVE ----------------

auto getPassword(const std::string& username) -> std::string {
    GError* error = nullptr;

    gchar* result = secret_password_lookup_sync(&mqtt_password_schema,
                                                nullptr, // cancellable
                                                &error, "username", username.c_str(), nullptr);

    if (error) {
        std::cerr << "[libsecret] lookup error: " << error->message << std::endl;
        g_error_free(error);
        return {};
    }

    if (!result)
        return {};

    std::string password(result);

    secret_password_free(result);
    return password;
}

void Credentials::storeCredentials(const std::string& username, const std::string& password) {
    storePassword(username, password);
}

auto Credentials::retrieveCredentials(const std::string& username) -> std::string {
    return getPassword(username);
}