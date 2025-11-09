package com.karolbystrek.mqttdatahub.repository;

import com.karolbystrek.mqttdatahub.model.Location;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.Optional;

@Repository
public interface LocationRepository extends JpaRepository<Location, Long> {
    Optional<Location> findByIdAndCustomerId(Long id, Long customerId);
}
