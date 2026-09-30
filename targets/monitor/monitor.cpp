/**
 * This program monitors the bus for a loss of heartbeat notifications from any
 * of the connected cameras.
 */

#include "heartbeat.h"
#include "httplib.hpp"
#include "json.hpp"
#include <csignal>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

void monitor_shutdown_handler(int signum) {
  std::cout << "[Monitor] Shutting down. Killing all cameras in group..."
            << std::endl;
  // Kills the monitor and any camera that explicitly joined this PGID
  kill(-getpgrp(), SIGTERM);
  _exit(signum);
}

void spawn_new_camera(int camera_id) {
  pid_t pid = fork();

  if (pid < 0) {
    perror("Fork failed");
    return;
  }

  if (pid == 0) { // --- CHILD PROCESS ---
    // Grab process group ID
    std::string pgid_str = std::to_string(getpgrp());
    std::string id_str = std::to_string(camera_id);

    // Build the arguments for camera
    char *args[] = {(char *)"./build/targets/camera/Camera",
                    (char *)id_str.c_str(), (char *)pgid_str.c_str(), nullptr};

    // Exec Camera
    execvp(args[0], args);

    // If execvp fails:
    perror("Failed to execute camera binary");
    _exit(1);
  }

  // --- PARENT PROCESS ---
  signal(SIGCHLD, SIG_IGN);
  std::cout << "[Monitor] Spawned recovery camera " << camera_id
            << " (PID: " << pid << ")" << std::endl;
}

int main(int argc, char **argv) {
  // setup process group, so we don't orphan any re-started camera processes.
  setpgid(0, 0);

  // setup handlers for termination signals, so we can kill the process group if
  // they happen
  signal(SIGINT, monitor_shutdown_handler);
  signal(SIGTERM, monitor_shutdown_handler);

  // Connect to the monitoring service.
  httplib::Client client("http://localhost:8129");

  constexpr int maximumDeadTime = 20;
  // We support a maximum of 100 heartbeat devices.
  constexpr int maximumDeviceCount = 100;

  // Active devices and their last check in time.
  // The device at activeDevices[i]'s last check in time is at
  // lastDeviceCheckInTime[i]
  int registeredDevices = 0;
  int activeDevices[maximumDeviceCount];
  time_t lastDeviceCheckInTime[maximumDeviceCount];

  std::fill_n(activeDevices, maximumDeviceCount, -1);
  std::fill_n(lastDeviceCheckInTime, maximumDeviceCount, -1);

  while (true) {
    // Read the bus to see if there are any new heartbeat messages
    if (auto result = client.Get("/read"); result && result->status == 200) {
      const auto [deviceID, timestamp] =
          nlohmann::json::parse(result->body).get<heartbeat::Message>();
      std::cout << "Got heartbeat from " << deviceID << " at: " << timestamp
                << std::endl;

      // If we have already seen this device, set its last check in time.
      bool foundDevice = false;
      for (int i = 0; i < maximumDeviceCount; i++) {
        if (activeDevices[i] == deviceID) {
          lastDeviceCheckInTime[i] = timestamp;
          foundDevice = true;
          break;
        }
      }

      if (!foundDevice) {
        activeDevices[registeredDevices] = deviceID;
        lastDeviceCheckInTime[registeredDevices] = timestamp;
        registeredDevices += 1;
      }
    }

    for (int i = 0; i < registeredDevices; i++) {
      if (difftime(time(nullptr), lastDeviceCheckInTime[i]) > maximumDeadTime) {
        std::cout << "Device " << activeDevices[i] << " has not checked in for "
                  << difftime(time(nullptr), lastDeviceCheckInTime[i])
                  << " seconds. Initiating Recovery..." << std::endl;

        // spawn a new camera
        spawn_new_camera(activeDevices[i]);
        // Give the new camera time to spawn, so that we dont start it again
        lastDeviceCheckInTime[i] = time(nullptr);
      }
    }
  }

  return 0;
}
