#pragma once

/**
 * heap-backed factories (std::unique_ptr, new) for queueManager. Fine on ESP32;
 * avoid in tight ESP8266 boot paths if you already use static queues.
 */
#include "queueManager.hpp"
#include <memory>
#include <type_traits>

class queueFactory {
public:
    // Templated generic creator for any packet type and size
    template<typename PacketType, size_t Size>
    static std::unique_ptr<queueManager<PacketType, Size>> createQueue() {
        static_assert(Size > 0 && (Size & (Size - 1)) == 0, "Queue size must be power of two");
        return std::unique_ptr<queueManager<PacketType, Size>>(new queueManager<PacketType, Size>());
    }

    // Factory with custom processor callback
    template<typename PacketType, size_t Size>
    static std::unique_ptr<queueManager<PacketType, Size>> createQueueWithProcessor(
        typename queueManager<PacketType, Size>::processorCallback processor) {
        std::unique_ptr<queueManager<PacketType, Size>> q(new queueManager<PacketType, Size>());
        q->setProcessor(processor);
        return q;
    }

    // Factory with custom filter callback
    template<typename PacketType, size_t Size>
    static std::unique_ptr<queueManager<PacketType, Size>> createQueueWithFilter(
        typename queueManager<PacketType, Size>::filterCallback filter) {
        std::unique_ptr<queueManager<PacketType, Size>> q(new queueManager<PacketType, Size>());
        q->setFilter(filter);
        return q;
    }

    // Factory with all callbacks
    template<typename PacketType, size_t Size>
    static std::unique_ptr<queueManager<PacketType, Size>> createQueueFull(
        typename queueManager<PacketType, Size>::processorCallback processor,
        typename queueManager<PacketType, Size>::filterCallback filter,
        typename queueManager<PacketType, Size>::overflowCallback overflow) {
        std::unique_ptr<queueManager<PacketType, Size>> q(new queueManager<PacketType, Size>());
        q->setProcessor(processor);
        q->setFilter(filter);
        q->setOverflowHandler(overflow);
        return q;
    }
};
