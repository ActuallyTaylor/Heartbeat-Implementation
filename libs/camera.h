#pragma once
#include "json.hpp"

namespace camera {

    enum class ObstacleStatus {
        CLEAR,
        CAUTION,
        WARNING,
        CRITICAL,
        FAULT,
        INVALID
    };

    struct CameraState {
        /// As the camera is exposed to the environment and mechanical vibration of the car it gradually wears down
        double cameraWear { 0.0 };
        bool failed = false;

        /// How many times the camera has run the detection cycle in this session.
        uint64_t detectionEvents { 0 };
    };

    struct ObstacleReading {
        double distanceMeters { 0.0 };
        double confidence { 0.0 };

        ObstacleStatus status{ObstacleStatus::CLEAR};
    };

    struct RecoveryState {
        uint64_t detectionEvents;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(RecoveryState, detectionEvents);
    };

}
