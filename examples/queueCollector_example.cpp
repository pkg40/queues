#include "queueCollector.hpp"
#include "transportPacket.hpp" // Replace with your actual packet type header
#include <iostream>

// Example usage of templated queueCollector
int main() {
    // Create a queueCollector for transportPacket
    queueCollector<transportPacket> qm({
        {"rxQueue", 8},
        {"txQueue", 16},
        {"eventQueue", 4}
    });

    // List all queues
    std::cout << "Queues available:" << std::endl;
    for (const auto& name : qm.listQueues()) {
        std::cout << "  " << name << std::endl;
    }

    // Access a specific queue (must know the size at compile time)
    auto* rxQ = qm.getQueue<8>("rxQueue");
    if (rxQ) {
        transportPacket pkt; // Fill with test data as needed
        rxQ->enqueue(pkt);
        std::cout << "Enqueued a packet to rxQueue." << std::endl;
    }

    // Access another queue
    auto* txQ = qm.getQueue<16>("txQueue");
    if (txQ) {
        transportPacket pkt;
        txQ->enqueue(pkt);
        std::cout << "Enqueued a packet to txQueue." << std::endl;
    }

    return 0;
}
