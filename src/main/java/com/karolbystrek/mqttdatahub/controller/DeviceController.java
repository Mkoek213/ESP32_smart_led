package com.karolbystrek.mqttdatahub.controller;

import com.karolbystrek.mqttdatahub.dto.DeviceStatusResponse;
import com.karolbystrek.mqttdatahub.model.Device;
import com.karolbystrek.mqttdatahub.service.DeviceService;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@Slf4j
@RestController
@RequestMapping("/api/devices")
@RequiredArgsConstructor
@Tag(name = "Devices", description = "API for managing devices")
public class DeviceController {

    private final DeviceService deviceService;

    @Operation(summary = "Get the latest status of a specific device")
    @GetMapping("/{deviceId}/status")
    public ResponseEntity<DeviceStatusResponse> getDeviceStatus(@PathVariable Long deviceId) {
        log.info("Received request to get status for deviceId: {}", deviceId);
        Device device = deviceService.getDeviceBy(deviceId);

        return ResponseEntity.ok(new DeviceStatusResponse(
                device.getId(),
                device.getStatus(),
                device.getUpdatedAt()
        ));
    }
}
