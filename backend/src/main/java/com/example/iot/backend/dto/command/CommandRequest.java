package com.example.iot.backend.dto.command;

import java.util.Map;

public record CommandRequest(
        String type,
        Map<String, Object> payload
) {
}
