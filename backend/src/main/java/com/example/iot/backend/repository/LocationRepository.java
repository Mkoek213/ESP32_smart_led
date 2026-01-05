package com.example.iot.backend.repository;

import com.example.iot.backend.model.Location;
import org.springframework.data.jpa.repository.JpaRepository;

import java.util.List;
import java.util.Optional;

public interface LocationRepository extends JpaRepository<Location, Long> {

    boolean existsByIdAndUserEmail(Long locationId, String userEmail);

    List<Location> findAllByUserId(Long userId);

    Optional<Location> findByIdAndUserId(Long locationId, Long userId);

    List<Location> findAllByUserIdAndNameContainingIgnoreCase(Long userId, String name);
}
