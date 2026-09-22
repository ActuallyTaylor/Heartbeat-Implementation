/**
 * A camera on the autonomous car that reports a heartbeat over the beat while it is live.
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iostream>
#include <random>
#include <thread>

#include "camera.h"
#include "heartbeat.h"
#include "httplib.hpp"
#include "json.hpp"

static const char* statusName(const camera::ObstacleStatus status) {
    switch (status) {
        case camera::ObstacleStatus::CLEAR:
            return "CLEAR";
        case camera::ObstacleStatus::CAUTION:
            return "CAUTION";
        case camera::ObstacleStatus::WARNING:
            return "WARNING";
        case camera::ObstacleStatus::CRITICAL:
            return "CRITICAL";
        case camera::ObstacleStatus::FAULT:
            return "FAULT";
        case camera::ObstacleStatus::INVALID:
            return "INVALID";
    }

    return "UNKNOWN";
}

static camera::ObstacleReading simulateNearestObstacle(const int deviceID, const std::time_t now) {
    const auto seedValue = static_cast<unsigned int>(
        static_cast<unsigned long long>(now) * 1000ULL +
        static_cast<unsigned long long>(static_cast<unsigned int>(deviceID)) * 37ULL);

    std::mt19937 rng(seedValue);
    std::uniform_real_distribution baseDistance(0.25, 12.0);
    std::uniform_real_distribution jitter(-0.45, 0.45);

    const double phase = (static_cast<double>(now) + static_cast<double>(deviceID)) * 0.7;
    double distance = baseDistance(rng) + jitter(rng) + std::sin(phase) * 0.8;
    distance = std::clamp(distance, 0.2, 25.0);

    if ((now + static_cast<std::time_t>(deviceID)) % 31 == 0) {
        return { .distance_m = -1.0, .confidence = 0.0, .status = camera::ObstacleStatus::FAULT };
    }
    if ((now + static_cast<std::time_t>(deviceID)) % 17 == 0) {
        return { .distance_m = 999.0, .confidence = 0.0, .status = camera::ObstacleStatus::INVALID };
    }

    double confidence = 0.96;
    auto status = camera::ObstacleStatus::CLEAR;

    if (distance < 0.75) {
        status = camera::ObstacleStatus::CRITICAL;
        confidence = 0.99;
    } else if (distance < 2.5) {
        status = camera::ObstacleStatus::WARNING;
        confidence = 0.95;
    } else if (distance < 6.0) {
        status = camera::ObstacleStatus::CAUTION;
        confidence = 0.90;
    }

    return { .distance_m = distance, .confidence = confidence, .status = status };
}

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        std::cerr << "You did not provide a device ID as the first argument." << std::endl;
        return 1;
    }

    const int deviceID = std::stoi(argv[1]);
    httplib::Client client("http://localhost:8129");

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));

        const std::time_t sendTime = std::time(nullptr);
        const camera::ObstacleReading obstacle = simulateNearestObstacle(deviceID, sendTime);

        if ((sendTime + static_cast<std::time_t>(deviceID)) % 23 == 0) {
            std::cerr << "Camera " << deviceID << " missed obstacle reading at " << sendTime << std::endl;
            continue;
        }

        const heartbeat::Message heartbeat {
            .deviceID = deviceID,
            .timestamp = sendTime
        };

        const nlohmann::json payload = {
            {"deviceID", heartbeat.deviceID},
            {"timestamp", heartbeat.timestamp}
        };

        if (const auto res = client.Post("/send", payload.dump(), "application/json"); !res || res->status != 200) {
            std::cerr << "Failed to send heartbeat: " << (res ? res->status : 0) << std::endl;
            continue;
        }

        std::cout << "Camera " << deviceID << " nearest obstacle: " << obstacle.distance_m
                  << " m [" << statusName(obstacle.status) << "]" << std::endl;
    }

    return 0;
}
