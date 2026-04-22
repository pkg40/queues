# Member Function Callbacks - Future Enhancement

## Overview

Currently, `queueManager` callbacks can be set using:
- Lambda functions (with or without captures)
- Free functions
- Function objects

**Future Enhancement**: Add explicit support and examples for using class member functions as callbacks, avoiding the need for lambda wrappers.

## Current Usage (Lambda Required)

```cpp
class PacketProcessor {
public:
    void processPacket(const MyPacket& pkt) {
        Serial.printf("Processing: %d\n", pkt.id);
    }
    
    void handleOverflow(const MyPacket& dropped) {
        Serial.printf("Overflow: %d\n", dropped.id);
    }
};

PacketProcessor processor;

// Current approach - requires lambda to capture 'this'
queueManager<MyPacket, 16> queue;
queue.setProcessor([&processor](const MyPacket& pkt) {
    processor.processPacket(pkt);  // Lambda wrapper needed
});
```

## Future Enhancement: Direct Member Function Support

### Option 1: Using std::bind (Recommended)

```cpp
#include <functional>

class PacketProcessor {
public:
    void processPacket(const MyPacket& pkt) {
        Serial.printf("Processing: %d\n", pkt.id);
    }
    
    void handleOverflow(const MyPacket& dropped) {
        Serial.printf("Overflow: %d\n", dropped.id);
    }
    
    bool filterPacket(const MyPacket& pkt) {
        return pkt.id != 0xFF;  // Filter invalid packets
    }
};

PacketProcessor processor;

queueManager<MyPacket, 16> queue;

// Direct member function binding - no lambda needed!
queue.setProcessor(std::bind(&PacketProcessor::processPacket, &processor, std::placeholders::_1));
queue.setFilter(std::bind(&PacketProcessor::filterPacket, &processor, std::placeholders::_1));
queue.setOverflowHandler(std::bind(&PacketProcessor::handleOverflow, &processor, std::placeholders::_1));
```

### Option 2: Using std::mem_fn (Alternative)

```cpp
#include <functional>

queue.setProcessor(std::mem_fn(&PacketProcessor::processPacket));
// Note: Requires passing 'this' pointer separately or using bind
```

### Option 3: Helper Template Method (Cleanest API)

Add to `queueManager`:

```cpp
// In queueManager.hpp - Future enhancement
template<typename Class, typename Method>
void setProcessor(Class* obj, Method method) {
    _processor = std::bind(method, obj, std::placeholders::_1);
}

template<typename Class, typename Method>
void setFilter(Class* obj, Method method) {
    _filter = std::bind(method, obj, std::placeholders::_1);
}

template<typename Class, typename Method>
void setOverflowHandler(Class* obj, Method method) {
    _overflow = std::bind(method, obj, std::placeholders::_1);
}
```

Then usage becomes:

```cpp
PacketProcessor processor;
queueManager<MyPacket, 16> queue;

// Clean, direct syntax - no lambdas, no std::bind in user code
queue.setProcessor(&processor, &PacketProcessor::processPacket);
queue.setFilter(&processor, &PacketProcessor::filterPacket);
queue.setOverflowHandler(&processor, &PacketProcessor::handleOverflow);
```

## Benefits

### ✅ Avoid Lambda Overhead
- No lambda closure allocation
- Direct function pointer binding
- Potentially better code generation

### ✅ Cleaner Code
- No need to capture `this` in lambdas
- More explicit about which method is being called
- Easier to read and maintain

### ✅ Better Performance
- `std::bind` with member function pointer is typically more efficient than lambda with capture
- Compiler can optimize better with direct function pointers

### ✅ Type Safety
- Compile-time checking of member function signatures
- Clearer error messages if signature doesn't match

## Implementation Notes

### Current Status
- ✅ `std::function` already supports `std::bind` results
- ✅ Template setters already accept any callable
- ⚠️ No explicit examples or helper methods for member functions

### Recommended Implementation

1. **Add helper template methods** (Option 3 above) for cleaner API
2. **Update documentation** with member function examples
3. **Add examples** in `examples/` directory
4. **Benchmark** lambda vs bind performance (likely negligible difference)

### Example Implementation

```cpp
// In queueManager.hpp - Add these methods:

// Convenience methods for member function callbacks
template<typename Class>
void setProcessor(Class* obj, void (Class::*method)(const packetType&)) {
    _processor = std::bind(method, obj, std::placeholders::_1);
}

template<typename Class>
void setFilter(Class* obj, bool (Class::*method)(const packetType&)) {
    _filter = std::bind(method, obj, std::placeholders::_1);
}

template<typename Class>
void setOverflowHandler(Class* obj, void (Class::*method)(const packetType&)) {
    _overflow = std::bind(method, obj, std::placeholders::_1);
}
```

## Comparison: Lambda vs std::bind

### Lambda Approach (Current)
```cpp
queue.setProcessor([this](const MyPacket& pkt) {
    this->processPacket(pkt);
});
```

**Pros:**
- Modern C++ style
- Inline, easy to read
- Can capture multiple variables

**Cons:**
- Lambda closure overhead
- Must capture `this` explicitly
- Less explicit about which method is called

### std::bind Approach (Future)
```cpp
queue.setProcessor(&processor, &PacketProcessor::processPacket);
```

**Pros:**
- Direct function pointer - no closure
- More explicit about method
- Potentially better optimization
- Cleaner syntax with helper methods

**Cons:**
- Requires `std::bind` (already in std::function)
- Slightly more verbose without helper methods

## Real-World Example

```cpp
class ESPNowReceiver {
private:
    queueManager<ESPNowPacket, 32> _rxQueue;
    
public:
    ESPNowReceiver() {
        // Direct member function binding - clean!
        _rxQueue.setProcessor(this, &ESPNowReceiver::handlePacket);
        _rxQueue.setFilter(this, &ESPNowReceiver::validatePacket);
        _rxQueue.setOverflowHandler(this, &ESPNowReceiver::logOverflow);
    }
    
    void handlePacket(const ESPNowPacket& pkt) {
        // Process received packet
        Serial.printf("Received from: %s\n", pkt.mac);
    }
    
    bool validatePacket(const ESPNowPacket& pkt) {
        // Filter invalid packets
        return pkt.length > 0 && pkt.length <= MAX_PACKET_SIZE;
    }
    
    void logOverflow(const ESPNowPacket& dropped) {
        // Log dropped packets
        Serial.printf("Queue overflow! Dropped packet from: %s\n", dropped.mac);
    }
    
    void loop() {
        _rxQueue.processAll();
    }
};
```

## Priority

**Status**: Future Enhancement (Low Priority)

**Rationale**:
- Current lambda approach works fine
- `std::bind` already works with current implementation
- Helper methods would improve ergonomics but aren't critical
- Can be added incrementally without breaking changes

## Action Items

1. ✅ Document current capability (std::bind works now)
2. ⏳ Add helper template methods for cleaner API
3. ⏳ Add examples to examples/ directory
4. ⏳ Update main README with member function examples
5. ⏳ Consider performance benchmarking (lambda vs bind)

## References

- C++ Member Function Pointers: https://isocpp.org/wiki/faq/pointers-to-members
- std::bind: https://en.cppreference.com/w/cpp/utility/functional/bind
- std::function: https://en.cppreference.com/w/cpp/utility/functional/function

