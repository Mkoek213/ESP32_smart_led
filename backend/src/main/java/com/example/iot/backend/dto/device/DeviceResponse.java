package com.example.iot.backend.dto.device;

import com.example.iot.backend.model.Device;
import lombok.Builder;

import java.time.LocalDateTime;

@Builder
public record DeviceResponse(
        Long id,
        String name,
        String status,
        LocalDateTime updatedAt,
        Long locationId) {
    public static DeviceResponse toDeviceResponse(Device device) {
        return DeviceResponse.builder()
                .id(device.getId())
                .name(device.getName())
                .status(device.getStatus().name())
                .updatedAt(device.getUpdatedAt())
                .locationId(device.getLocation().getId())
                .build();
    }
}
