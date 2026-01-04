package com.example.iot.backend.dto;

import java.util.Map;

public record Command(
        String type,
        Map<String, Object> payload
) {
}
