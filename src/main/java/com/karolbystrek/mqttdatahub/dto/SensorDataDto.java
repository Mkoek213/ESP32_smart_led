package com.karolbystrek.mqttdatahub.dto;

import java.time.LocalDateTime;

public record SensorDataDto(
        LocalDateTime timestamp,
        Double humidity,
        Boolean motionDetected,
        Integer lightLevel
) {
}
