package com.example.iot.backend.dto.telemetry;

import com.example.iot.backend.model.Telemetry;
import lombok.Builder;

@Builder
public record TelemetryResponse(
        Long id,
        TelemetryDto payload,
        Long deviceId
) {
    public static TelemetryResponse toTelemetryResponse(Telemetry data) {
        return TelemetryResponse.builder()
                .id(data.getId())
                .deviceId(data.getDevice().getId())
                .payload(TelemetryDto.toTelemetryDto(data))
                .build();
    }
}
