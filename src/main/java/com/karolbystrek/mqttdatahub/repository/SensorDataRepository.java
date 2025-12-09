package com.karolbystrek.mqttdatahub.repository;

import com.karolbystrek.mqttdatahub.model.SensorData;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;

@Repository
public interface SensorDataRepository extends JpaRepository<SensorData, Long> {

    List<SensorData> findByDeviceIdIn(List<Long> deviceId);
}
