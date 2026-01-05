package com.example.iot.backend.controller;

import com.example.iot.backend.dto.device.DeviceCreateRequest;
import com.example.iot.backend.dto.device.DeviceResponse;
import com.example.iot.backend.service.DeviceService;
import jakarta.validation.Valid;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.http.HttpStatus;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.ResponseStatus;
import org.springframework.web.bind.annotation.RestController;

@Slf4j
@RestController
@RequestMapping("/devices")
@RequiredArgsConstructor
public class DeviceController {

    private final DeviceService deviceService;

    @GetMapping("/{deviceId}")
    @PreAuthorize("authorizationService.canAccessDevice(principal, #deviceId)")
    public DeviceResponse getDevice(@PathVariable Long deviceId) {
        log.info("Received request to get device with id: {}", deviceId);
        return deviceService.getDevice(deviceId);
    }

    @PostMapping
    @ResponseStatus(HttpStatus.CREATED)
    @PreAuthorize("authorizationService.canAccessLocation(principal, #request.locationId)")
    public DeviceResponse createDevice(@RequestBody @Valid DeviceCreateRequest request) {
        log.info("Received request to create device: {}", request);
        return deviceService.createDevice(request);
    }

    @GetMapping("/{deviceId}/status")
    @PreAuthorize("authorizationService.canAccessDevice(principal, #deviceId)")
    public DeviceResponse getDeviceStatus(@PathVariable Long deviceId) {
        log.info("Received request to get status for deviceId: {}", deviceId);
        return deviceService.getDevice(deviceId);
    }
}
