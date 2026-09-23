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

# Implementation Diagram Narrative
![Heartbeat Implementation Diagram.png](docs/Heartbeat%20Implementation%20Diagram.png)

This project is a mock implementation of a use-case of the "Heartbeat" availability tactic in the context of an
autonomous vehicle. The Implementation Diagram shows the separate modules we created and how they interact. As a summary, 
three separate types of processes are created, each mimicking one component within the object detection subsystem in an
autonomous vehicle. First, a "camera" process mimics the actual object detection, with random chance failures to 
demonstrate the utility of the heartbeat tactic. Each camera publishes its heartbeat pings to the second process, an HTTP 
server serving as a mock CAN bus, which stores those pings and their timestamps in a message queue to mimic the queue-like
properties of the real CAN bus. Finally, a Monitor process repeatedly checks the bus for pings, tracking active heartbeat
devices and the timestamps of their most recent pings. 

## Camera Processes
The Camera processes are relatively simple. Each Camera process enters a loop on startup, first waiting for a delay
period then performing mock object detection with randomly generated object distances. Once a distance is generated, the
Camera sends a message via an HTTP POST endpoint (chosen because of the ease of portability between operating systems) 
to the Bus. This message is the "heartbeat" after which the tactic is named, announcing the health of the process. The
Cameras also have a non-deterministic failure chance, which allows for the failure detection of the Monitor to be
showcased. 

## Bus Process 
The Bus process is a mock version of the CAN bus in an autonomous vehicle, the bus on which a real heartbeat might be
sent. In our case, the Bus is imitated using an HTTP server with an internal message queue. This server serves two
endpoints, a POST endpoint for Camera processes to send their heartbeat pings and a GET endpoint for the Monitor to use
to receive heartbeat pings. When a Camera sends a ping via the POST endpoint, the Bus places it on an internal message
queue, from which messages are taken and sent to the Monitor with each GET request. This mimics the queueing behavior of
a real CAN bus, and allows for multiple clients on different processes or even different machines to be handled
concurrently for the demo. 

## Monitor Process
The Monitor represents the central monitor of a real heartbeat system, constantly checking and tracking the status of
the "beating hearts" within the system. Our Monitor accomplishes this by repeatedly sending GET requests to the Bus to
pull heartbeats off of the message queue. With each message received, the Monitor first checks if the pinging device has
been seen before, saving the deviceID and initial ping time if not. If the device has been seen before, the Monitor
updates the most recently received heartbeat timestamp for that device, and moves on. If any devices have not sent a
heartbeat after a configurable timeout, the Monitor announces as such by printing a message to the console. 
