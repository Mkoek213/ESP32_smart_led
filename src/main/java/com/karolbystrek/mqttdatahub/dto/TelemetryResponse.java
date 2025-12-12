package com.karolbystrek.mqttdatahub.dto;

import com.karolbystrek.mqttdatahub.model.Telemetry;
import lombok.Builder;

@Builder
public record TelemetryResponse(
        Long id,
        Long deviceId,
        TelemetryDto payload
) {
    public static TelemetryResponse fromSensorData(Telemetry data) {
        return TelemetryResponse.builder()
                .id(data.getId())
                .deviceId(data.getDevice().getId())
                .payload(TelemetryDto.createFrom(data))
                .build();
    }
}
