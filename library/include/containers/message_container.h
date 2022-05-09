#ifndef MESSAGE_CONTAINER_H
#define MESSAGE_CONTAINER_H

#include <string>

/**
 * @brief Container class for all data
 * @details Parent class for all other data types
 */
class message_container{
public:
    /**
     * @brief Get container content as byte array
     * @details
     * @note
     * @return byte array representation with contents of the container
     */
    virtual auto byte_array() -> std::string = 0;

    /**
     * @brief Get container content as json
     * @details
     * @note
     * @return json formatted string with contents of the container
     */
    virtual auto json() -> std::string = 0;
};

#endif // MESSAGE_CONTAINER_H