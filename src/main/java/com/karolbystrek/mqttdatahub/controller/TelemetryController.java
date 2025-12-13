package com.karolbystrek.mqttdatahub.controller;

import com.karolbystrek.mqttdatahub.dto.TelemetryResponse;
import com.karolbystrek.mqttdatahub.service.TelemetryService;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.web.bind.annotation.*;

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
    @PostMapping(value = "/{customerId}", consumes = "application/json", produces = "application/json")
    public Map<Long, List<TelemetryResponse>> get(@PathVariable Long customerId, @RequestBody List<Long> deviceIds) {
        log.info("Received request to get telemetry for customerId: {} and deviceIds: {}", customerId, deviceIds);
        return telemetryService.getTelemetryFor(customerId, deviceIds);
    }
}
