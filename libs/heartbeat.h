//
// Created by Taylor Lineman on 9/20/26.
//

#ifndef HEARTBEAT_IMPLEMENTATION_HEARTBEAT_H
#define HEARTBEAT_IMPLEMENTATION_HEARTBEAT_H
#include "json.hpp"

namespace heartbeat {
    struct Message {
        int deviceID;
        std::time_t time;
    };

    // Generates to_json/from_json.
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Message, deviceID, time)
}

#endif //HEARTBEAT_IMPLEMENTATION_HEARTBEAT_H
