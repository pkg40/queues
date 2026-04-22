// queueManager Testbench
// Validates enqueue, dequeue, overflow, and callback logic
#include <Arduino.h>
#include <queueManager.hpp>
#include <deviceDataPacket.hpp>

queueManager<deviceDataPacket, 4> testQueue;

void testProcessor(const deviceDataPacket& pkt) {
    Serial.printf("[TEST] Processed opcode: 0x%02X\n", pkt.opcode);
}

void testOverflow(const deviceDataPacket& dropped) {
    Serial.printf("[TEST] Overflow dropped opcode: 0x%02X\n", dropped.opcode);
}

void setup() {
    Serial.begin(115200);
    testQueue.setProcessor(testProcessor);
    testQueue.setOverflowHandler(testOverflow);
    // Fill queue and trigger overflow
    for (uint8_t i = 0; i < 6; ++i) {
        deviceDataPacket pkt = { .opcode = i };
        testQueue.enqueue(pkt);
    }
}

void loop() {
    testQueue.processAll();
    delay(100);
}
