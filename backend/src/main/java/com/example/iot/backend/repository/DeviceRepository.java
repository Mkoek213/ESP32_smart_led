package com.example.iot.backend.repository;

import com.example.iot.backend.model.Device;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;
import java.util.Optional;

@Repository
public interface DeviceRepository extends JpaRepository<Device, Long> {

    Optional<Device> findByIdAndLocationUserId(Long deviceId, Long userId);

    List<Device> findAllByLocationUserId(Long locationUserId);

    boolean existsByIdAndLocationUserId(Long deviceId, Long userId);
}
