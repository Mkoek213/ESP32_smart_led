#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <deque>
#include <mutex>
#include <vector>
#include <cstdint>

struct Telemetry {
  int64_t timestamp;
  double temperature;
  double humidity;
  double pressure;
  bool motion_detected;
};

class SensorManager {
public:
  static SensorManager &getInstance();

  void enqueue(const Telemetry &data);
  void requeueBatch(const std::vector<Telemetry>& batch);
  std::vector<Telemetry> popBatch(size_t max_count);
  size_t size() const;
  bool empty() const;

private:
  SensorManager() = default;
  ~SensorManager() = default;
  SensorManager(const SensorManager &) = delete;
  SensorManager &operator=(const SensorManager &) = delete;

  std::deque<Telemetry> queue_;
  mutable std::mutex mutex_;
  const size_t MAX_QUEUE_SIZE = 1000; // Limit memory usage
};

#endif // SENSOR_MANAGER_H
