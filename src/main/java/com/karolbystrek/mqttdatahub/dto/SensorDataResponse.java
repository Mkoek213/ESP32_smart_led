package com.karolbystrek.mqttdatahub.dto;

import com.karolbystrek.mqttdatahub.model.SensorData;
import lombok.Builder;

import java.time.LocalDateTime;

@Builder
public record SensorDataResponse(
        Long id,
        Long deviceId,
        LocalDateTime createdAt,
        Payload payload
) {
    public static SensorDataResponse fromSensorData(SensorData data) {
        return SensorDataResponse.builder()
                .id(data.getId())
                .deviceId(data.getDevice().getId())
                .createdAt(data.getCreatedAt())
                .payload(Payload.fromSensorData(data))
                .build();
    }

    @Builder
    public record Payload(
            LocalDateTime timestamp,
            Double humidity,
            Boolean motionDetected,
            Integer lightLevel
    ) {

        private static Payload fromSensorData(SensorData data) {
            return Payload.builder()
                    .timestamp(data.getTimestamp())
                    .humidity(data.getHumidity())
                    .motionDetected(data.getMotionDetected())
                    .lightLevel(data.getLightLevel())
                    .build();
        }
    }
}
