# Design decisions

# Language, Libraries and Framework

## Why C++?

Initially I developed this app using a C# frontend (using WPF) and a C backend for calling the C APIs exposed by Thorlabs Kinesis and NI-DAQmx. But I found it annoying that you had to expose the backend as a dll and manually managing the underlying device (opening, closing etc.) was cumbersome. 

Therefore, I decided to use C++, because not only can C++ call the APIs directly, I can also give each acquired resource an owner and manage it automatically using RAII. The frontend can also be written in C++ so that it is all managed as one source rather than two large and seperate components.


## Why Qt 6?

I mainly chose Qt because it seemed like it was the most popular open source option for developing a UI in a Windows environment with C++. 


## Architecture

### Why manage worker lifetimes outside the UI?

I considered letting `MainWindow` own the worker threads. It could start them and shut them down in the required order. I chose a separate `WorkerManager` to keep thread lifetimes separate from the UI. The window queues operator requests and displays replies. The manager starts the workers and waits for their threads to finish when the application exits.

Shutdown illustrates the boundary. The scan worker needs the motor and laser workers available while it stops X/Y motion and turns the output off. The manager therefore shuts down the scan worker first. If that dependency changes, the shutdown code can change independently of the window and its controls. [Worker manager](../src/workers/worker_manager/worker_manager.cc)

### Why give each axis and the scan worker their own threads?

When repositioning with the laser off, X and Y can move concurrently even if one worker issues both commands. A move request starts motion on the controller. The worker can issue the second move before the first finishes, then poll both channels and track completion separately.

The current design gives each axis worker its own event loop, pending request, polling timer, and timeout. This keeps per-axis state local and lets Y’s event loop process requests while X’s thread is inside an SDK call. A single controller worker could simplify thread management and serialize SDK access, but a slow call on either channel would delay polling and command delivery for both. Both designs use the same physical controller. [Motor worker](../src/workers/motor_worker/motor_worker.cc)

The scan worker decides when the next instruction may start. In the same repositioning example, it waits for *both* completion replies before requesting laser ON. Its own event loop lets it receive a cancellation or handle a timeout while a motor worker is busy. That keeps scan progress and recovery independent of any one axis and of the GUI event loop. [Scan worker](../src/workers/scan_worker/scan_worker.cc)

## Why plan the scan before motion starts?

This was a idea that came up after my conversation with Charlie (a previous student working on this project). I thought it was much better software engineering practice to have a clear seperation of concern between route planning and route execution. It would mean the code is more modular and easier to test.
