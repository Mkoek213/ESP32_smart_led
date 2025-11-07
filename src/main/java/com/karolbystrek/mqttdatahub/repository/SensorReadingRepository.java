package com.karolbystrek.mqttdatahub.repository;

import com.karolbystrek.mqttdatahub.model.SensorReading;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

@Repository
public interface SensorReadingRepository extends JpaRepository<SensorReading, Long> {
}
