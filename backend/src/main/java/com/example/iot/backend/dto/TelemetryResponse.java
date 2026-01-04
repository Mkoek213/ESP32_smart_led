package com.example.iot.backend.dto;

import com.example.iot.backend.model.Telemetry;
import lombok.Builder;

@Builder
public record TelemetryResponse(
        Long id,
        Long deviceId,
        TelemetryDto payload
) {
    public static TelemetryResponse from(Telemetry data) {
        return TelemetryResponse.builder()
                .id(data.getId())
                .deviceId(data.getDevice().getId())
                .payload(TelemetryDto.from(data))
                .build();
    }
}
