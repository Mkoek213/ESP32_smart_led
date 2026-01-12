package com.example.iot.backend.repository;

import com.example.iot.backend.model.FirmwareUpdate;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.Optional;

@Repository
public interface FirmwareRepository extends JpaRepository<FirmwareUpdate, Long> {
    Optional<FirmwareUpdate> findFirstByOrderByCreatedAtDesc();

    Optional<FirmwareUpdate> findByVersion(String version);
}
