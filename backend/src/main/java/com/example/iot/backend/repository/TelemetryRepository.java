package com.example.iot.backend.repository;

import com.example.iot.backend.model.Telemetry;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;

@Repository
public interface TelemetryRepository extends JpaRepository<Telemetry, Long> {

    List<Telemetry> findAllByDeviceLocationUserIdAndTimestampBetween(Long userId, Long startTimestamp,
            Long endTimestamp);

    List<Telemetry> findAllByDeviceIdAndTimestampBetween(Long deviceId, Long startTimestamp, Long endTimestamp);

    java.util.Optional<Telemetry> findFirstByDeviceIdOrderByTimestampDesc(Long deviceId);

    void deleteAllByDeviceId(Long deviceId);
}
