#include <gpio_mapping.h>
#include <map>
#include <config.h>

std::map<GPIO_PIN, unsigned int> GPIO_PINMAP = GPIO_PINMAP_VERSIONS[MUONPI_DEFAULT_HW_VERSION];
