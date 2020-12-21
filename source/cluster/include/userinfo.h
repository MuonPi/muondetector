#ifndef USERINFO_H
#define USERINFO_H

#include <string>


namespace MuonPi {

    struct UserInfo
    {
        std::string username {};
        std::string station_id {};

        /**
         * @brief site_id Creates a site_id string from the username and the station id.
         * @return The site id as string
         */
        [[nodiscard]] auto site_id() const -> std::string { return username+station_id; }
    };
}
#endif // USERINFO_H