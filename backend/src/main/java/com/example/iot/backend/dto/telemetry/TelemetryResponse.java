package com.example.iot.backend.dto.telemetry;

import com.example.iot.backend.model.Telemetry;
import lombok.Builder;

@Builder
public record TelemetryResponse(
        Long id,
        Long deviceId,
        TelemetryDto payload
) {
    public static TelemetryResponse toTelemetryResponse(Telemetry data) {
        return TelemetryResponse.builder()
                .id(data.getId())
                .deviceId(data.getDevice().getId())
                .payload(TelemetryDto.from(data))
                .build();
    }
}
