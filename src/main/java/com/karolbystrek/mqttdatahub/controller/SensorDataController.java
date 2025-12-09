package com.karolbystrek.mqttdatahub.controller;

import com.karolbystrek.mqttdatahub.dto.SensorDataResponse;
import com.karolbystrek.mqttdatahub.service.SensorDataService;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;
import java.util.Map;

@Slf4j
@RestController
@RequestMapping("/api/sensor-data")
@RequiredArgsConstructor
public class SensorDataController {

    private final SensorDataService sensorDataService;

    @PostMapping(value = "/{customerId}", consumes = "application/json", produces = "application/json")
    public Map<Long, List<SensorDataResponse>> getSensorData(@PathVariable Long customerId, @RequestBody List<Long> deviceIds) {
        log.info("Received request to get sensor data for customerId: {} and deviceIds: {}", customerId, deviceIds);
        return sensorDataService.getSensorDataGroupedByDeviceId(customerId, deviceIds);
    }
}
