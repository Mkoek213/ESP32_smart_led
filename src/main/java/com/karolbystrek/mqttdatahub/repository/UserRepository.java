package com.karolbystrek.mqttdatahub.repository;

import com.karolbystrek.mqttdatahub.model.User;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

@Repository
public interface UserRepository extends JpaRepository<User, Long> {
}
