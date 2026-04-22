#pragma once

/**
 * queueCollector — named map of queueManager instances (optional utility).
 *
 * Embedded note: uses std::map, std::string, std::shared_ptr, and std::vector
 * (heap). Prefer explicit queueManager globals (e.g. deviceQueues) on small
 * targets unless you need this dynamic registry.
 *
 * getQueue<Size>() must use the same Size used when the queue was created; the
 * stored pointer is type-erased (shared_ptr<void>) and a wrong Size is UB.
 */
#include "queueFactory.hpp"
#include <memory>
#include <map>
#include <string>
#include <initializer_list>
#include <vector>

template<typename PacketType>
class queueCollector {
public:
    struct QueueSpec {
        std::string name;
        size_t size;
        QueueSpec(const std::string& n, size_t s) : name(n), size(s) {}
    };

    using QueuePtr = std::shared_ptr<void>; //  pointer for generic storage
    using QueueMap = std::map<std::string, QueuePtr>;

private:
    QueueMap _queues;

public:
    // Construct with a list of queue specs (name, size)
    queueCollector(std::initializer_list<QueueSpec> specs = {
        {"rxQueue", 8},
        {"txQueue", 8},
        {"commandQueue", 16},
        {"sensorQueue", 4},
        {"eventQueue", 16}
    }) {
        for (const auto& spec : specs) {
            _queues[spec.name] = createQueue(spec.size);
        }
    }

    // Create a queueManager<PacketType, N> for a given size
    static QueuePtr createQueue(size_t size) {
        switch (size) {
            case 4:
                return std::shared_ptr<void>(queueFactory::createQueue<PacketType, 4>().release());
            case 8:
                return std::shared_ptr<void>(queueFactory::createQueue<PacketType, 8>().release());
            case 16:
                return std::shared_ptr<void>(queueFactory::createQueue<PacketType, 16>().release());
            case 32:
                return std::shared_ptr<void>(queueFactory::createQueue<PacketType, 32>().release());
            default:
                return nullptr;
        }
    }

    // Get a pointer to a queue by name and size (user must know the size)
    template<size_t Size>
    queueManager<PacketType, Size>* getQueue(const std::string& name) {
        auto it = _queues.find(name);
        if (it != _queues.end()) {
            return static_cast<queueManager<PacketType, Size>*>(it->second.get());
        }
        return nullptr;
    }

    // List all queue names
    std::vector<std::string> listQueues() const {
        std::vector<std::string> names;
        for (const auto& kv : _queues) names.push_back(kv.first);
        return names;
    }
};