package com.karolbystrek.mqttdatahub.repository;

import com.karolbystrek.mqttdatahub.model.Location;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.Optional;

@Repository
public interface LocationRepository extends JpaRepository<Location, Long> {
    Optional<Location> findByNameAndCustomerId(String name, Long customerId);
}
