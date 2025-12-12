#include "sensor_manager.h"

SensorManager& SensorManager::getInstance() {
    static SensorManager instance;
    return instance;
}

void SensorManager::enqueue(const Telemetry& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.size() >= MAX_QUEUE_SIZE) {
        // Drop oldest if full to prevent OOM
        queue_.pop_front();
    }
    queue_.push_back(data);
}

void SensorManager::requeueBatch(const std::vector<Telemetry>& batch) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = batch.rbegin(); it != batch.rend(); ++it) {
        if (queue_.size() < MAX_QUEUE_SIZE) {
            queue_.push_front(*it);
        }
        else {
            queue_.push_front(*it);
            queue_.pop_back(); 
        }
    }
}

std::vector<Telemetry> SensorManager::popBatch(size_t max_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Telemetry> batch;
    batch.reserve(max_count);

    while (!queue_.empty() && batch.size() < max_count) {
        batch.push_back(queue_.front());
        queue_.pop_front();
    }
    return batch;
}

size_t SensorManager::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

bool SensorManager::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}
