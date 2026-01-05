package com.example.iot.backend.dto.telemetry;

import com.example.iot.backend.model.Telemetry;
import lombok.Builder;

@Builder
public record TelemetryDto(
        Long timestamp,
        Double temperature,
        Double humidity,
        Double pressure,
        Integer personCount
) {
    public static TelemetryDto toTelemetryDto(Telemetry telemetry) {
        return TelemetryDto.builder()
                .timestamp(telemetry.getTimestamp())
                .temperature(telemetry.getTemperature())
                .humidity(telemetry.getHumidity())
                .pressure(telemetry.getPressure())
                .personCount(telemetry.getPersonCount())
                .build();
    }
}
