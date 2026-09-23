#pragma once

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
        double camera_wear = 0.0; // As the camera is exposed to the environment and mechanical vibration of the car it gradually wears down
        bool failed = false;
    };

    struct ObstacleReading {
        double distance_m{0.0};
        double confidence{0.0};
        ObstacleStatus status{ObstacleStatus::CLEAR};
    };

}
