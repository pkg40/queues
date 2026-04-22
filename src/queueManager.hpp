/* ************************************************************************************
 * queueManager.hpp - Type-Safe Queue with Callbacks and Filtering
 ************************************************************************************
 * 
 * OVERVIEW:
 * =========
 * A higher-level queue wrapper around ringBuffer that adds:
 *   - Processor callbacks (batch drain via processAll(), not on dequeue)
 *   - Filter callbacks (conditional enqueue; reject before queue touch)
 *   - Overflow callbacks (oldest item passed here when making room on full)
 *   - Convenience aliases (push/pop alongside enqueue/dequeue)
 * 
 * This is the primary queue type used throughout the application for:
 *   - Sensor data queues (sensorPacket_t)
 *   - Handler/command queues (commandPacket_t)
 *   - Transport queues (transportPacket_t)
 *   - Event queues (EVENT_t wrapped)
 * 
 * USAGE PATTERNS:
 * ===============
 * 
 * 1. SIMPLE QUEUE (no callbacks):
 *    
 *    queueManager<sensorPacket_t, 8> sensorQ;
 *    sensorQ.enqueue(packet);        // Add to queue
 *    sensorPacket_t out;
 *    if (sensorQ.dequeue(out)) {     // Remove from queue
 *        process(out);
 *    }
 * 
 * 2. WITH PROCESSOR (drain + invoke via processAll()):
 *    
 *    queueManager<sensorPacket_t, 8> sensorQ;
 *    sensorQ.setProcessor([](const sensorPacket_t& pkt) {
 *        updateDisplay(pkt.core.data.fData);
 *    });
 *    sensorQ.processAll();  // Drains queue, calls processor for each
 * 
 * 3. WITH FILTER (conditional enqueue):
 *    
 *    queueManager<commandPacket_t, 8> cmdQ;
 *    cmdQ.setFilter([](const commandPacket_t& cmd) {
 *        return cmd.handler.opcode != HANDLER_NOP;  // Ignore NOP commands
 *    });
 *    cmdQ.enqueue(packet);  // Only enqueued if filter returns true
 * 
 * 4. WITH OVERFLOW HANDLER:
 *    
 *    queueManager<eventPacket_t, 4> eventQ;
 *    eventQ.setOverflowHandler([](const eventPacket_t& dropped) {
 *        LOG_WARN(LOG_CAT_SYSTEM, "Event dropped: %d", dropped.eventId);
 *    });
 * 
 * THREAD SAFETY (inherits ringBuffer, adds non-atomic sequences)
 * ==============================================================
 * Each underlying ringBuffer operation (push/pop/isFull/…) is protected by the
 * same critical-section rules as ringBuffer.hpp. This class adds logic between
 * those calls:
 *
 *   - _filter(packet) runs with no queue lock. Do not touch the queue from the
 *     filter callback.
 *   - _overflow(dropped) runs after a pop that made room for the new item; it is
 *     outside ringBuffer’s critical section (fine for logging; avoid heavy work
 *     in ISR-driven systems).
 *   - enqueue() when full: isFull → pop (oldest) → optional overflow → push.
 *     Those steps are separate critical sections. With multiple concurrent
 *     producers, another writer can interleave; treat “drop oldest” as best
 *     suited to a single producer or externally serialized enqueue path.
 *   - processAll() drains with repeated pops; another context can push between
 *     pops — this is intentional “best effort” draining, not a snapshot.
 *
 * CALLBACK SEMANTICS (read carefully)
 * ===================================
 *
 *   Processor:  Invoked only from processAll(), once per removed item.
 *               Signature: void(const packetType&)
 *               NOT called from dequeue() / pop() — call processAll() or handle
 *               the packet yourself after dequeue().
 *
 *   Filter:     Runs at the start of enqueue(). Return true to continue, false
 *               to drop the packet with no enqueue and no overflow callback.
 *               Signature: bool(const packetType&)
 *
 *   Overflow:   When the queue is already full, enqueue() removes the oldest
 *               item, then calls _overflow with that removed value, then pushes
 *               the new packet. So this is “drop oldest to make room”, not
 *               “reject because push failed” (raw ringBuffer still fails push if
 *               full; queueManager proactively makes room first).
 *
 * EMBEDDED COMPATIBILITY
 * ======================
 *   - packetType must stay trivially copyable; ringBuffer stores memcpy’d slots.
 *   - std::function may allocate when assigned a capturing lambda or a large
 *     functor. Prefer stateless lambdas, std::ref + function pointers, or no
 *     callbacks if you must avoid heap churn on ESP8266-class heaps.
 *   - Callbacks must respect ISR/task context: no Serial/blocking in paths that
 *     run from interrupt if your enqueue runs there.
 *   - Drop-oldest under concurrency: see THREAD SAFETY above.
 *
 * PERFORMANCE NOTES
 * =================
 *   - Callback storage is three std::function objects (size/overhead depends on
 *     STL implementation; often small-buffer optimized for tiny callables).
 *   - Power-of-two bufferSize matches ringBuffer fast wrap; other sizes work.
 *   - Templates avoid virtual calls on the hot push/pop path.
 *
 * MEMORY (rough footprint)
 * ========================
 *   sizeof(ringBuffer<packetType,bufferSize>) + 3 × sizeof(std::function<…>)
 * 
 ************************************************************************************
 * Copyright (c)       2019-2025 Peter K Green            - pkg40@yahoo.com
 ************************************************************************************
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
 * Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ************************************************************************************/

#pragma once
#include "ringBuffer.hpp"
#include <cstddef>
#include <utility>
#include <functional>

template<typename packetType, size_t bufferSize>
class queueManager {
    static_assert(bufferSize > 0, "bufferSize must be greater than zero");
    
    // Power-of-two optimization hint (optional, not required)
    // Power-of-two sizes (2, 4, 8, 16, 32, 64, 128, 256) enable bitwise modulo optimization
    // in the underlying ringBuffer for slightly better performance. Any size > 0 is supported.
    // This is a compile-time constant that can be used for optimization hints but does not
    // restrict the buffer size - non-power-of-two sizes work correctly.
    static constexpr bool _isPowerOfTwo = (bufferSize > 0) && ((bufferSize & (bufferSize - 1)) == 0);

public:
    /** One-line heap snapshot after queue/callback setup (ESP8266 only). */
    static void printHeapStatus(const char* context = "queueManager") {
        #ifdef ESP8266
            Serial.printf("[%s] Free heap: %u bytes, Max block: %u bytes\n", context,
                          ESP.getFreeHeap(), ESP.getMaxFreeBlockSize());
        #endif
    }
    using processorCallback = std::function<void(const packetType&)>;
    using filterCallback = std::function<bool(const packetType&)>;
    using overflowCallback = std::function<void(const packetType&)>;

    queueManager() = default;

    // Add a constructor that accepts a processor callback
    explicit queueManager(processorCallback processor)
        : _processor(std::move(processor)) {}

    // Template-based setters (zero overhead)
    template<typename F>
    void setProcessor(F&& f);

    template<typename F>
    void setFilter(F&& f) ;

    template<typename F>
    void setOverflowHandler(F&& f) ; 

    /** Enqueue; false if filter rejects. On full queue, drops oldest then pushes. */
    bool enqueue(const packetType& packet);
    bool push(const packetType& packet) {
        return enqueue(packet);
    }

    bool dequeue(packetType& packet);
    bool pop(packetType& packet) {
        return dequeue(packet);
    }

    /** Drain queue; invokes processor for each item if set. */
    void processAll();

    bool isEmpty() const;
    bool isFull() const ;
    size_t size() const ;
    size_t capacity() const ;
    void clear() ;

private:
    ringBuffer<packetType, bufferSize> _queue;
    processorCallback _processor = nullptr;
    filterCallback _filter = nullptr;
    overflowCallback _overflow = nullptr;
};

#include "queueManager.ipp"