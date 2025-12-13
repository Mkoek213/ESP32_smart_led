package com.karolbystrek.mqttdatahub.controller;

import com.karolbystrek.mqttdatahub.dto.CommandRequest;
import com.karolbystrek.mqttdatahub.service.DeviceCommandService;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

@Slf4j
@RestController
@RequestMapping("/api/commands")
@RequiredArgsConstructor
@Tag(name = "Device Commands", description = "API for sending commands to devices")
public class DeviceCommandController {

    private final DeviceCommandService deviceCommandService;

    @Operation(summary = "Send a command to a specific device")
    @PostMapping("/customer/{customerId}/location/{locationId}/device/{deviceId}")
    public ResponseEntity<Void> sendCommand(
            @PathVariable Long customerId,
            @PathVariable Long locationId,
            @PathVariable Long deviceId,
            @RequestBody CommandRequest request
    ) {
        log.info("Received command request for device {}: {}", deviceId, request);
        deviceCommandService.sendCommand(customerId, locationId, deviceId, request);
        return ResponseEntity.accepted().build();
    }
}

