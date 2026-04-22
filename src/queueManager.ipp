#pragma once

/**
 * queueManager implementation — kept in .ipp so the header stays readable.
 * enqueue() full path: filter → if full, pop oldest → overflow(dropped) → push new.
 */

template <typename packetType, size_t bufferSize>
template <typename F>
void queueManager<packetType, bufferSize>::setProcessor(F&& f) {
    _processor = std::forward<F>(f);
}

template <typename packetType, size_t bufferSize>
template <typename F>
void queueManager<packetType, bufferSize>::setFilter(F&& f) {
    _filter = std::forward<F>(f);
}

template <typename packetType, size_t bufferSize>
template <typename F>
void queueManager<packetType, bufferSize>::setOverflowHandler(F&& f) {
    _overflow = std::forward<F>(f);
}

template <typename packetType, size_t bufferSize>
bool queueManager<packetType, bufferSize>::enqueue(const packetType& packet) {
    if (_filter && !_filter(packet)) {
        return false;
    }

    if (_queue.isFull()) {
        packetType dropped;
        _queue.pop(dropped);
        if (_overflow) {
            _overflow(dropped);
        }
    }

    return _queue.push(packet);
}

template <typename packetType, size_t bufferSize>
bool queueManager<packetType, bufferSize>::dequeue(packetType& packet) {
    return _queue.pop(packet);
}

template <typename packetType, size_t bufferSize>
void queueManager<packetType, bufferSize>::processAll() {
    if (!_processor) {
        return;
    }

    packetType packet;
    while (_queue.pop(packet)) {
        _processor(packet);
    }
}

template <typename packetType, size_t bufferSize>
bool queueManager<packetType, bufferSize>::isEmpty() const {
    return _queue.isEmpty();
}

template <typename packetType, size_t bufferSize>
bool queueManager<packetType, bufferSize>::isFull() const {
    return _queue.isFull();
}

template <typename packetType, size_t bufferSize>
size_t queueManager<packetType, bufferSize>::size() const {
    return _queue.size();
}

template <typename packetType, size_t bufferSize>
size_t queueManager<packetType, bufferSize>::capacity() const {
    return _queue.capacity();
}

template <typename packetType, size_t bufferSize>
void queueManager<packetType, bufferSize>::clear() {
    _queue.clear();
}
