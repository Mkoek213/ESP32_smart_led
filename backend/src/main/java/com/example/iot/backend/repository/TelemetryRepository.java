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

    @org.springframework.data.jpa.repository.Modifying(clearAutomatically = true, flushAutomatically = true)
    @org.springframework.data.jpa.repository.Query("DELETE FROM Telemetry t WHERE t.device.id = :deviceId")
    void deleteAllByDeviceId(@org.springframework.data.repository.query.Param("deviceId") Long deviceId);
}
