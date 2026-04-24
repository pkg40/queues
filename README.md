# queues

Type-safe, templated queue and ring-buffer library for Arduino/ESP32 and native C++ projects.

## Overview

Three complementary classes cover different needs:

| Class | Description |
|-------|-------------|
| `ringBuffer<T, N>` | Bare-metal FIFO; ISR-safe push/pop; no callbacks |
| `queueManager<T, N>` | Wraps `ringBuffer`; adds processor, filter, and overflow callbacks |
| `queueCollector<T>` | Manages a named set of `queueManager` instances via factory pattern |

## Features

- Fully templated — works with any trivially-copyable data type
- Compile-time capacity with optional power-of-two fast wrap
- Drop-oldest overflow policy (queueManager)
- Lambda/function-pointer callbacks for processing, filtering, and overflow
- Native (PC) and embedded (ESP32/ESP8266) compatible
- Zero dynamic allocation in the queue storage itself

## Quick Start

```cpp
#include <queueManager.hpp>

struct MyPacket { int id; float value; };

queueManager<MyPacket, 8> q;
q.setProcessor([](const MyPacket& p) { Serial.printf("id=%d\n", p.id); });

q.enqueue({1, 3.14f});
q.processAll();    // drain and invoke processor
```

## Directory Structure

```
src/   — ringBuffer.hpp, queueManager.hpp, queueCollector.hpp, queueFactory.hpp
docs/  — README.md (design), queueManager_analysis.md, MEMBER_FUNCTION_CALLBACKS.md
```

See [USERS_GUIDE.md](USERS_GUIDE.md) for full API documentation.
