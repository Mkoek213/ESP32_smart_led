package com.karolbystrek.mqttdatahub.dto;

import com.karolbystrek.mqttdatahub.model.Telemetry;
import lombok.Builder;

@Builder
public record TelemetryDto(
        Long timestamp,
        Boolean motionDetected,
        Double temperature,
        Double humidity,
        Double pressure
) {
    public static TelemetryDto createFrom(Telemetry telemetry) {
        return TelemetryDto.builder()
                .timestamp(telemetry.getTimestamp())
                .motionDetected(telemetry.getMotionDetected())
                .temperature(telemetry.getTemperature())
                .humidity(telemetry.getHumidity())
                .pressure(telemetry.getPressure())
                .build();
    }
}
