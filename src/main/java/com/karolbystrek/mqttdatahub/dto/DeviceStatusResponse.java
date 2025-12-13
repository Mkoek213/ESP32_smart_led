package com.karolbystrek.mqttdatahub.dto;

import java.time.LocalDateTime;

public record DeviceStatusResponse(
        Long deviceId,
        String status,
        LocalDateTime updatedAt
) {
}

