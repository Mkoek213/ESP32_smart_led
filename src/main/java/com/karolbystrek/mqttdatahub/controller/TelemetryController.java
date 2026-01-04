package com.karolbystrek.mqttdatahub.controller;

import com.karolbystrek.mqttdatahub.dto.TelemetryResponse;
import com.karolbystrek.mqttdatahub.service.TelemetryService;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
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
@RequestMapping("/api/telemetry")
@RequiredArgsConstructor
@Tag(name = "Telemetry", description = "API for retrieving telemetry data")
public class TelemetryController {

    private final TelemetryService telemetryService;

    @Operation(summary = "Get telemetry for specific devices")
    @PostMapping(value = "/{userId}", consumes = "application/json", produces = "application/json")
    public Map<Long, List<TelemetryResponse>> get(@PathVariable Long userId, @RequestBody List<Long> deviceIds) {
        log.info("Received request to get telemetry for user with id: {} and device ids: {}", userId, deviceIds);
        return telemetryService.getTelemetryFor(userId, deviceIds);
    }
}
