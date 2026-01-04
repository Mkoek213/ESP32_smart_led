package com.example.iot.backend.repository;

import com.example.iot.backend.model.Location;
import org.springframework.data.jpa.repository.JpaRepository;

public interface LocationRepository extends JpaRepository<Location, Long> {
    boolean existsByIdAndUser_Email(Long locationId, String userEmail);
}
