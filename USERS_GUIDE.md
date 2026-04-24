# queues — User Guide

## Choosing the Right Class

| Need | Use |
|------|-----|
| Raw fast FIFO, possibly from ISR | `ringBuffer<T,N>` |
| FIFO with callbacks (process/filter/overflow) | `queueManager<T,N>` |
| Multiple named queues managed together | `queueCollector<T>` |

---

## ringBuffer — Low-Level FIFO

```cpp
#include <ringBuffer.hpp>

ringBuffer<uint8_t, 16> buf;   // 16-slot circular buffer

// Producer (may be ISR):
uint8_t val = 42;
buf.push(val);          // returns false if full

// Consumer (loop):
uint8_t item;
while (buf.pop(item)) {
    process(item);
}
```

### API

| Method | Returns | Description |
|--------|---------|-------------|
| `push(const T&)` | `bool` | Add to tail; false if full |
| `pop(T&)` | `bool` | Remove from head; false if empty |
| `peek(T&)` | `bool` | Copy head without removing |
| `isEmpty()` | `bool` | True when no items |
| `isFull()` | `bool` | True when at capacity |
| `size()` | `size_t` | Current item count |
| `capacity()` | `size_t` | Maximum items (`N`) |
| `clear()` | void | Discard all items |

---

## queueManager — Queue with Callbacks

### Simple (no callbacks)

```cpp
#include <queueManager.hpp>

queueManager<sensorPacket_t, 8> sensorQ;

sensorQ.enqueue(packet);          // alias: push()

sensorPacket_t out;
if (sensorQ.dequeue(out)) {       // alias: pop()
    handleSensor(out);
}
```

### Processor callback (drain pattern)

```cpp
sensorQ.setProcessor([](const sensorPacket_t& pkt) {
    updateDisplay(pkt.value);
});

void loop() {
    sensorQ.processAll();   // dequeues all pending items and calls the processor
}
```

### Filter callback (conditional enqueue)

```cpp
cmdQ.setFilter([](const commandPacket_t& cmd) {
    return cmd.opcode != CMD_NOP;   // reject NOP before it touches the queue
});
```

### Overflow callback (drop-oldest)

When `enqueue()` is called on a full queue, the oldest item is removed to make room.
The overflow callback receives the dropped item:

```cpp
eventQ.setOverflowHandler([](const eventPacket_t& dropped) {
    LOG_WARN("Event dropped: %d", dropped.eventId);
});
```

### Callback execution order on `enqueue()`

1. Filter is called — if it returns `false`, enqueue stops here, no queue touch.
2. If queue is full: oldest item is popped, overflow callback invoked.
3. New item is pushed.

### Callback execution on `processAll()`

`processAll()` loops calling `dequeue()` internally and invokes the processor for each item.
The processor is **not** called by `dequeue()` alone.

### API

| Method | Description |
|--------|-------------|
| `enqueue(packet)` / `push(packet)` | Enqueue with filter/overflow handling |
| `dequeue(out)` / `pop(out)` | Remove head item |
| `processAll()` | Drain + invoke processor |
| `setProcessor(fn)` | `void(const T&)` callback |
| `setFilter(fn)` | `bool(const T&)` callback |
| `setOverflowHandler(fn)` | `void(const T&)` callback |
| `isEmpty()` / `isFull()` | State queries |
| `size()` / `capacity()` | Count queries |
| `clear()` | Discard all items |

---

## queueCollector — Named Queue Registry

Use when multiple queues of the same data type need to be looked up by name at runtime:

```cpp
#include <queueCollector.hpp>

// Factory creates named queues with different capacities
queueCollector<MyPacket> qc({
    {"rxQueue", 8},
    {"txQueue", 16}
});

auto* rxQ = qc.getQueue<8>("rxQueue");
if (rxQ) rxQ->enqueue(packet);
```

`getQueue<N>(name)` returns a typed pointer to the `queueManager<T,N>` or `nullptr` if not found.

---

## Sizing Guidelines

| Queue depth | When to use |
|-------------|-------------|
| 4–8 | Commands, events — very low rate |
| 8–16 | Sensor data at 10–50 Hz |
| 32–64 | Transport/serial bursts |
| 128+ | High-rate data with bursty producers |

Prefer **power-of-two** sizes (4, 8, 16, 32 …) — the underlying `ringBuffer` optimises the modulo wrap with a bitwise AND when the size is a power of two.

---

## Thread Safety Notes

- Each individual `push()` / `pop()` is atomic at the ringBuffer level.
- `queueManager::enqueue()` performs `isFull → pop → overflow → push` as separate steps — not an atomic sequence. Safe with a single producer; use external serialisation for multiple concurrent producers.
- `processAll()` is a best-effort drain — producers can push between pops.
- Callbacks (`filter`, `overflow`, `processor`) run outside the queue lock — keep them short and non-blocking.

---

## Embedded Compatibility

- `packetType` must be **trivially copyable** — the ring buffer stores `memcpy`-safe slots.
- Avoid capturing lambdas with large closures on ESP8266 heaps; prefer stateless lambdas or free-function pointers.
- Do not call `Serial.print` or blocking operations from callbacks that run in ISR context.

---

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| Items silently dropped | Filter rejecting them | Add a debug log in the filter lambda |
| `processAll()` never fires callback | Processor not set | Call `setProcessor()` before `processAll()` |
| Queue always full | Depth too small | Increase `N` template parameter |
| `getQueue<N>()` returns nullptr | Wrong name or wrong `N` | Name must match exactly; `N` must match the factory-configured size |
