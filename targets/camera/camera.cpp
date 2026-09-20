/**
 * A camera on the autonomous car that reports a heartbeat over the beat while it is live.
 */

#include <iostream>

#include "httplib.hpp"
#include "json.hpp"
#include "heartbeat.h"
#include <chrono>

int main(int argc, char *argv[]) {
    if(argc <= 1) {
        std::cerr << "You did not provide a device ID as the first argument." << std::endl;
        exit(1);
    }

    int deviceID = std::stoi(argv[1]);

    httplib::Client client("http://localhost:8129");

    while (1) {
        // TODO: Do camera detection stuff here per assignment requirement  #2
        // TODO: Something should fail here randomly per assignment requirement #3

        // Send a heartbeat every 10 seconds, in the future replace with some form of "camera" functions from assignment document.
        sleep(10);
        auto sendTime = std::time(nullptr);

        heartbeat::Message haertbeat {
            .deviceID = deviceID,
            .timestamp = sendTime
        };

        nlohmann::json j = haertbeat;
        auto res = client.Post("/send", j.dump(), "application/json");

        if (res->status != 200) {
            std::cerr << "Failed to send heartbeat: " << res->status << std::endl;
        }
    }

    return 0;
}
