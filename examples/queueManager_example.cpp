// queueManager Example Usage
// Demonstrates basic queue operations and callback setup
#include <Arduino.h>
#include <queueManager.hpp>
#include <deviceDataPacket.hpp>

queueManager<deviceDataPacket, 8> rxQueue;

void processPacket(const deviceDataPacket& pkt) {
    Serial.printf("[EXAMPLE] Received opcode: 0x%02X\n", pkt.opcode);
}

void setup() {
    Serial.begin(115200);
    rxQueue.setProcessor(processPacket);
    deviceDataPacket pkt = { .opcode = 0x42 };
    rxQueue.enqueue(pkt);
}

void loop() {
    rxQueue.processAll();
    delay(100);
}
