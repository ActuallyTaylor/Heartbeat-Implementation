/**
 * This program monitors the bus for a loss of heartbeat notifications from any of the connected
 * cameras.
 */

#include <iostream>
#include <vector>
#include "httplib.hpp"
#include "json.hpp"
#include "heartbeat.h"

int main(int argc, char** argv) {
    // Connect to the monitoring service.
    httplib::Client client("http://localhost:8129");

    // TODO: Update these with arguments
    // A heartbeat should be seen by every device at the max every 10 seconds.
    int maximumDeadTime = 10;
    // We support a maximum of 100 heartbeat devices.
    int maximumDeviceCount = 100;

    // Active devices and their last check in time.
    // The device at activeDevices[i]'s last check in time is at lastDeviceCheckInTime[i]
    int registeredDevices = 0;
    int activeDevices[maximumDeviceCount];
    time_t lastDeviceCheckInTime[maximumDeviceCount];

    std::fill_n(activeDevices, maximumDeviceCount, -1);
    std::fill_n(lastDeviceCheckInTime, maximumDeviceCount, -1);

    while (1) {
        // Read the bus to see if there are any new heartbeat messages
        auto result = client.Get("/read");

        if (result && result->status == 200) {
            auto message = nlohmann::json::parse(result->body).get<heartbeat::Message>();
            std::cout << "Got heartbeat from " << message.deviceID << " at: " << message.timestamp << std::endl;

            // If we have already seen this device, set its last check in time.
            bool foundDevice = false;
            for (int i = 0; i < maximumDeviceCount; i++) {
                if (activeDevices[i] == message.deviceID) {
                    lastDeviceCheckInTime[i] = message.timestamp;
                    foundDevice = true;
                    break;
                }
            }

            if (!foundDevice) {
                activeDevices[registeredDevices] = message.deviceID;
                lastDeviceCheckInTime[registeredDevices] = message.timestamp;
                registeredDevices += 1;
            }
        }

        for (int i = 0; i < registeredDevices; i++) {
            if (difftime(time(nullptr), lastDeviceCheckInTime[i]) > maximumDeadTime) {
                std::cout << "Device " << activeDevices[i] << " has not checked in for " << difftime(time(nullptr), lastDeviceCheckInTime[i]) << " seconds" << std::endl;
            }
        }
    }

    return 0;
}
