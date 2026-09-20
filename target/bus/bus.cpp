/**
 * A bus that emulates a CAN bus on an autonomous vehicle. Messages are received and place into a queue
 * until they are read by another device.
 */

#include "httplib.hpp"
#include "json.hpp"

int main() {
    httplib::Server svr;

    svr.Post("/send", [&](const auto& req, auto& res) {
        auto json = nlohmann::json::parse(res.body);
        auto id = json["device_id"].get<int>();

    });

    svr.listen("0.0.0.0", 8080);

    return 0;
}
