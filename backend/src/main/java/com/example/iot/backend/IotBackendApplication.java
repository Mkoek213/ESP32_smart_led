package com.example.iot.backend;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.integration.annotation.IntegrationComponentScan;

@SpringBootApplication
@IntegrationComponentScan()
public class IotBackendApplication {

    static void main(String[] args) {
        SpringApplication.run(IotBackendApplication.class, args);
    }
}
