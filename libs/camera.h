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

struct ObstacleReading {
    double distance_m{0.0};
    double confidence{0.0};
    ObstacleStatus status{ObstacleStatus::CLEAR};
};

}
