# Heartbeat Tactic Implementation
This project represents a mock implementation of a use-case of the "Heartbeat" availability tactic in the context of an
autonomous vehicle. Three separate types of processes are created, each described below and each mimicking one component within
the object detection subsystem in an autonomous vehicle. First, a "camera" process mimics the actual object detection,
with random chance failures to demonstrate the use of the heartbeat tactic. Each camera publishes its heartbeat pings to
the second process, an HTTP server serving as a mock CAN bus, which stores those pings and their timestamps in a
message queue mimicking the queue-like properties of the real CAN bus. Finally, a Monitor process repeatedly checks the 
bus for pings regularly, tracking active heartbeat devices and the timestamps of their most recent pings.

# Build Instructions

## Operating System Requirements
This project comes shipped with an automated build script to automate the build and startup of all processes needed for
the demo. This script is written assuming a Unix-based operating system, meaning that Windows is not natively supported
by our build script. On Windows, consider running this project from within the [Windows Subsystem for Linux
(WSL)](https://learn.microsoft.com/en-us/windows/wsl/install). 

## Running The Build Script 
Before running the build script, make sure CMake is installed and at a version >=4.3. Then, simply run `./build.sh` to
automatically compile the targets for the demo and start, in separate processes, the Bus, the Monitor, and three
Cameras. The Cameras will then publish heartbeat pings to the Bus, which will be viewed by the Monitor. Camera processes
will also show some mock output imitating real object detection behavior. 

## Building Without the Script
To build the app without the script, use the following CMake commands:
```
cmake -B build
cmake --build build
```

This will compile the CPP files for the project. Output directories for each targtet will be present in the `build/targets` 
directory, with each including executable object modules for its respective target. Each process
can be started directly from the command-line within the corresponding directory. The Bus process should be started 
first, then the Monitor and any Cameras can be started in separate terminal windows. When running the Camera processes, 
note that a device ID must be provided as a command line argument. 
