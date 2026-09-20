/**
 * A bus that emulates a CAN bus on an autonomous vehicle. Messages are received and place into a queue
 * until they are read by another device.
 */

#include "httplib.hpp"
#include "json.hpp"
#include "heartbeat.h"

int main() {
    httplib::Server svr;

    std::vector<heartbeat::Message> messageQueue;

    svr.Get("/read", [&](const httplib::Request& req, httplib::Response& res) {
        if (!messageQueue.empty()) {
            // Get the message at the end of the queue, this will be the latest message on the bus.
            heartbeat::Message back = messageQueue.back();
            messageQueue.pop_back();

            nlohmann::json j = back;
            res.status = 200;
            res.body = j.dump();
        } else {
            // No content response code.c
            res.status = 204;
        }
    });


    svr.Post("/send", [&](const httplib::Request& req, httplib::Response& res) {
        auto message = nlohmann::json::parse(req.body).get<heartbeat::Message>();

        // Add the message to the beginning of the queue, so we can easily pop the queue when reading.
        messageQueue.insert(messageQueue.begin(), message);
        res.status = 200;
        std::cout << "Received heartbeat from: " << message.deviceID << " at: " << message.timestamp << std::endl;
    });

    svr.listen("0.0.0.0", 8129);

    return 0;
}
