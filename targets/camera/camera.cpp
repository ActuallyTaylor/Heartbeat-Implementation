/**
 * A camera on the autonomous car that reports a heartbeat over the beat while
 * it is live.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <unistd.h>

#include "camera.h"
#include "heartbeat.h"
#include "httplib.hpp"
#include "json.hpp"

constexpr double MAX_WEAR = 100.0;
constexpr std::string CHECKPOINT_FOLDER = "checkpoints";
constexpr std::string CHECKPOINT_FILE_PREFIX = "camera_checkpoint_";

static const char *statusName(const camera::ObstacleStatus status) {
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

static camera::ObstacleReading
simulateNearestObstacle(const int deviceID, const std::time_t now,
                        camera::CameraState &state, std::mt19937 &rng) {
  std::uniform_real_distribution randomPower(0.0, 1.0);

  const double wearIncrease =
      0.1 +
      4.9 * std::pow(randomPower(rng), 2.5); // Randomly simulates wear from 0.1
                                             // to 5%, biased towards 0.1%

  state.cameraWear += wearIncrease; // Adds to accumulated camera wear

  // Camera fails completely when it reaches the maximum wear threshold
  if (state.cameraWear >= MAX_WEAR) {
    state.cameraWear = MAX_WEAR;
    state.failed = true;

    return {.distanceMeters = -1.0,
            .confidence = 0.0,
            .status = camera::ObstacleStatus::FAULT};
  }

  std::uniform_real_distribution invalidRandom(0.0, 1.0);
  double invalidReadingProbability = 0.0;

  // As camera wear accumulates, the risk of an invalid reading increases
  if (state.cameraWear >= 75.0) {
    invalidReadingProbability = 0.10;
  } else if (state.cameraWear >= 50.0) {
    invalidReadingProbability = 0.5;
  } else if (state.cameraWear >= 25.0) {
    invalidReadingProbability = 0.01;
  }

  if (invalidRandom(rng) < invalidReadingProbability) {
    return {.distanceMeters = 999.0,
            .confidence = 0.0,
            .status = camera::ObstacleStatus::INVALID};
  }

  const auto seedValue = static_cast<unsigned int>(
      static_cast<unsigned long long>(now) * 1000ULL +
      static_cast<unsigned long long>(static_cast<unsigned int>(deviceID)) *
          37ULL);

  std::mt19937 obstacleRandom(seedValue);

  std::uniform_real_distribution baseDistance(0.25, 12.0);
  std::uniform_real_distribution jitter(-0.45, 0.45);

  const double phase =
      (static_cast<double>(now) + static_cast<double>(deviceID)) * 0.7;
  double distance = baseDistance(obstacleRandom) + jitter(obstacleRandom) +
                    std::sin(phase) * 0.8;
  distance = std::clamp(distance, 0.2, 25.0);

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

  return {
      .distanceMeters = distance, .confidence = confidence, .status = status};
}

static void deleteCameraState(const int deviceID) {
  const std::string checkpointName =
      CHECKPOINT_FILE_PREFIX + std::to_string(deviceID);
  const std::filesystem::path checkPointPath = std::filesystem::current_path()
                                                   .append(CHECKPOINT_FOLDER)
                                                   .append(checkpointName);

  std::filesystem::remove(checkPointPath);
}

static void saveCameraState(const camera::RecoveryState &status,
                            const int deviceID) {
  const std::string checkpointName =
      CHECKPOINT_FILE_PREFIX + std::to_string(deviceID);
  const std::filesystem::path checkPointPath = std::filesystem::current_path()
                                                   .append(CHECKPOINT_FOLDER)
                                                   .append(checkpointName);

  // Create the checkpoints file.
  std::filesystem::create_directory(checkPointPath.parent_path());

  // Create a file stream that will write over the existing checkpoint
  std::ofstream checkpointFile(checkPointPath);

  const nlohmann::json payload = status;

  checkpointFile << payload.dump();
  checkpointFile.close();
}

static std::optional<camera::RecoveryState>
loadExistingCameraState(const int deviceID) {
  const std::string checkpointName =
      CHECKPOINT_FILE_PREFIX + std::to_string(deviceID);

  if (const std::filesystem::path checkPointPath =
          std::filesystem::current_path()
              .append(CHECKPOINT_FOLDER)
              .append(checkpointName);
      std::filesystem::exists(checkPointPath)) {
    std::ifstream checkPointFile(checkPointPath);
    auto state =
        nlohmann::json::parse(checkPointFile).get<camera::RecoveryState>();

    return state;
  }

  return std::nullopt;
}

// This is an atomic global so that it can be used within the signal handler for
// termination
std::atomic<int> deviceID;

void termination_handler(int signum) {
  deleteCameraState(deviceID.load());
  _exit(signum);
}

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    std::cerr << "You did not provide a device ID as the first argument."
              << std::endl;
    return 1;
  }

  deviceID.store(std::stoi(argv[1]));
  httplib::Client client("http://localhost:8129");

  if (argc >= 3) {

    const int grouppid = std::stoi(argv[2]);
    std::cout << "[Camera" << deviceID
              << "] Invoked with process group ID. Attempting to join process "
                 "group."
              << std::endl;
    if (setpgid(0, grouppid) == 0) {
      std::cout << "[Camera" << deviceID
                << "] Successfully joined monitor process group." << std::endl;
    } else {
      std::cout << "[Camera" << deviceID << "] Failed to join process group."
                << std::endl;
    }
  }

  // Set termination_handler up to handle SIGINT and SIGTERM
  signal(SIGINT, termination_handler);
  signal(SIGTERM, termination_handler);

  std::random_device rd;
  std::mt19937 rng(rd());

  camera::CameraState cameraState;

  // Recover any previous checkpoints that a failed camera may have saved.
  if (const auto recoveryState = loadExistingCameraState(deviceID)) {
    cameraState.detectionEvents = recoveryState->detectionEvents;
    std::cout << "Recovered camera state for device " << deviceID << ": "
              << cameraState.detectionEvents << " detection events."
              << std::endl;
  }

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(2));

    const std::time_t sendTime = std::time(nullptr);
    const camera::ObstacleReading obstacle =
        simulateNearestObstacle(deviceID, sendTime, cameraState, rng);

    if (cameraState.failed) {
      std::cerr << "Camera " << deviceID << " has failed completely. "
                << std::endl;
      break;
    }

    // As the wear on the camera increases, the risk of missed readings also
    // increases
    double missedReadingProbability = 0.0;

    if (cameraState.cameraWear >= 75.0) {
      missedReadingProbability = 0.25;
    } else if (cameraState.cameraWear >= 50.0) {
      missedReadingProbability = 0.10;
    } else if (cameraState.cameraWear >= 25) {
      missedReadingProbability = 0.01;
    }

    // If the camera is still operational, readings may still be missed based on
    // wear thresholds
    if (std::uniform_real_distribution missedChance(0.0, 1.0);
        !cameraState.failed && missedChance(rng) < missedReadingProbability) {
      std::cerr << "Camera " << deviceID << " missed obstacle reading at "
                << sendTime << std::endl;
      continue;
    }

    cameraState.detectionEvents += 1;
    // If the camera has received a good reading, save its state into a
    // checkpoint.
    saveCameraState(camera::RecoveryState{.detectionEvents =
                                              cameraState.detectionEvents + 1},
                    deviceID);

    const heartbeat::Message heartbeat{.deviceID = deviceID,
                                       .timestamp = sendTime};

    const nlohmann::json payload = {{"deviceID", heartbeat.deviceID},
                                    {"timestamp", heartbeat.timestamp}};

    if (const auto res =
            client.Post("/send", payload.dump(), "application/json");
        !res || res->status != 200) {
      std::cerr << "Failed to send heartbeat: " << (res ? res->status : 0)
                << std::endl;
      continue;
    }

    std::cout << "Camera " << deviceID
              << " nearest obstacle: " << obstacle.distanceMeters << " m ["
              << statusName(obstacle.status) << "]" << std::endl;
  }

  // If the camera did not fatally fail, delete the camera state since we don't
  // care about it after the capture session.
  if (!cameraState.failed) {
    deleteCameraState(deviceID);
  }

  return 0;
}
