#include <cstring>
#include <gpiod.h>
#include <iostream>
#include <poll.h>
#include <vector>

int main() {
    const char* chipPath = "/dev/gpiochip0";

    // Put ALL input GPIOs you want to monitor here
    std::vector<unsigned int> lines = {
        18, 22, 27, 5, 12, 16, 23 // adjust as needed
    };

    gpiod_chip* chip = gpiod_chip_open(chipPath);
    if (!chip) {
        std::cerr << "Failed to open chip\n";
        return 1;
    }

    // --- configure line settings ---
    gpiod_line_settings* settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);

    // IMPORTANT for floating signals
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);

    gpiod_line_config* line_cfg = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(line_cfg, lines.data(), lines.size(), settings);

    gpiod_request_config* req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "gpiotest");

    gpiod_line_request* req = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    if (!req) {
        std::cerr << "Failed to request lines\n";
        return 1;
    }

    int fd = gpiod_line_request_get_fd(req);
    if (fd < 0) {
        std::cerr << "Failed to get fd\n";
        return 1;
    }

    std::cout << "Monitoring GPIOs...\n";

    gpiod_edge_event_buffer* buffer = gpiod_edge_event_buffer_new(128);

    struct pollfd pfd {};
    pfd.fd = fd;
    pfd.events = POLLIN;

    while (true) {
        int ret = poll(&pfd, 1, -1); // block forever

        if (ret < 0) {
            std::cerr << "poll error: " << strerror(errno) << "\n";
            break;
        }

        if (!(pfd.revents & POLLIN))
            continue;

        int n = gpiod_line_request_read_edge_events(req, buffer, 128);

        for (int i = 0; i < n; i++) {
            auto* ev = gpiod_edge_event_buffer_get_event(buffer, i);

            unsigned int line = gpiod_edge_event_get_line_offset(ev);

            auto type = gpiod_edge_event_get_event_type(ev);

            auto ts = gpiod_edge_event_get_timestamp_ns(ev);

            std::cout << "GPIO " << line << " | "
                      << (type == GPIOD_EDGE_EVENT_RISING_EDGE ? "RISING" : "FALLING")
                      << " | ts=" << ts << std::endl;
        }
    }

    gpiod_edge_event_buffer_free(buffer);
    gpiod_line_request_release(req);
    gpiod_chip_close(chip);

    return 0;
}