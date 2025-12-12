package com.karolbystrek.mqttdatahub.repository;

import com.karolbystrek.mqttdatahub.model.Telemetry;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;

@Repository
public interface TelemetryRepository extends JpaRepository<Telemetry, Long> {

    List<Telemetry> findByDeviceIdIn(List<Long> deviceId);
}
