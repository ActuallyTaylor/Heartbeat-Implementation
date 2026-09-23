# Implementation Diagram Narrative

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
