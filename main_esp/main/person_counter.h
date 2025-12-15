#ifndef PERSON_COUNTER_H
#define PERSON_COUNTER_H

#include <stdint.h>

/**
 * @brief Thread-safe person counter using FreeRTOS mutex
 * 
 * This class provides a thread-safe way to track person count across
 * multiple tasks (distance sensor task and BLE sensor task).
 */
class PersonCounter {
public:
    /**
     * @brief Initialize the person counter (must be called before use)
     */
    static void init();
    
    /**
     * @brief Increment the person count by 1 (thread-safe)
     */
    static void increment();
    
    /**
     * @brief Get current count and reset to 0 atomically (thread-safe)
     * @return Current person count before reset
     */
    static int get_and_reset();
    
    /**
     * @brief Get current count without resetting (thread-safe)
     * @return Current person count
     */
    static int get();

private:
    static void* mutex;  // SemaphoreHandle_t (void* for header portability)
    static int count;
};

#endif // PERSON_COUNTER_H
