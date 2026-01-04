package com.example.iot.backend.repository;

import com.example.iot.backend.model.Telemetry;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;

@Repository
public interface TelemetryRepository extends JpaRepository<Telemetry, Long> {

    List<Telemetry> findAllByDevice_Location_User_Id(Long userId);
}
