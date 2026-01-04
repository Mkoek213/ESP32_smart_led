package com.karolbystrek.mqttdatahub.repository;

import com.karolbystrek.mqttdatahub.model.Device;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.Optional;

@Repository
public interface DeviceRepository extends JpaRepository<Device, Long> {
    Optional<Device> findByIdAndLocationId(Long id, Long locationId);

    boolean existsByIdAndLocationUserId(Long deviceId, Long userId);
}
