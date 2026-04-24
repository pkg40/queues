# queues Library

Reusable, type-safe queueManager and queueCollector C++ library for embedded and native projects.

## Overview
- **queueManager**: A fixed-size, type-safe, circular queue for any packet or data type.
- **queueCollector**: A factory-driven manager for collections of named queues, allowing dynamic queue sets and easy access by name.

## Features
- Templated for any data type
- Compile-time size selection
- Named queue collections
- Factory pattern for queue creation
- Designed for embedded (Arduino/ESP32) and native (PC) use

## Directory Structure
- `src/` — All core headers and implementations
- `examples/` — Usage and test examples
- `docs/` — Design analysis, user manual, and API documentation

## Getting Started
See the User Manual for setup, usage, and API details.

---

# User Manual

## 1. Installation
Copy the `queues` directory to your PlatformIO `lib/` folder, or add it as a library dependency if published.

## 2. Basic Usage
### Creating a Single Queue
```cpp
#include "queueManager.hpp"
queueManager<MyPacket, 8> myQueue;
myQueue.enqueue(packet);
```

### Using queueCollector
```cpp
#include "queueCollector.hpp"
queueCollector<MyPacket> qc({{"rxQueue", 8}, {"txQueue", 16}});
auto* rxQ = qc.getQueue<8>("rxQueue");
if (rxQ) rxQ->enqueue(packet);
```

## 3. API Reference
- **queueManager<T, N>**: Fixed-size queue for type T, size N
  - `enqueue(const T&)`, `dequeue(T&)`, `size()`, `capacity()`, etc.
- **queueCollector<T>**: Manages multiple named queues
  - `getQueue<N>(name)`, `listQueues()`, etc.

## 4. Examples
See the `examples/` directory for full code samples.

## 5. Advanced
- Extend with custom queue types via the factory
- Integrate with event-driven or RTOS systems

## 6. License
MIT

---

For design details, see `docs/queueManager_analysis.md`.
