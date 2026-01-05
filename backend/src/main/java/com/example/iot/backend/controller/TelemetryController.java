package com.example.iot.backend.controller;

import com.example.iot.backend.dto.telemetry.TelemetryResponse;
import com.example.iot.backend.service.TelemetryService;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;
import java.util.Map;

@Slf4j
@RestController
@RequestMapping("/telemetry")
@RequiredArgsConstructor
public class TelemetryController {

    private final TelemetryService telemetryService;

    @PostMapping(value = "/{userId}")
    public Map<Long, List<TelemetryResponse>> get(@PathVariable Long userId) {
        log.info("Received request to get telemetry for user with id {}", userId);
        return telemetryService.getTelemetry(userId);
    }
}
