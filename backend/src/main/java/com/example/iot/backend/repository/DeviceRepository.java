package com.example.iot.backend.repository;

import com.example.iot.backend.model.Device;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

@Repository
public interface DeviceRepository extends JpaRepository<Device, Long> {

    boolean existsByIdAndLocation_User_Email(Long deviceId, String email);
}
