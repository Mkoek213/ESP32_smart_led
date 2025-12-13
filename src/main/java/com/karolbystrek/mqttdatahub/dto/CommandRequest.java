package com.karolbystrek.mqttdatahub.dto;

import io.swagger.v3.oas.annotations.media.Schema;

import java.util.Map;

@Schema(description = "Request object for sending commands to a device")
public record CommandRequest(
        @Schema(description = "Type of the command", example = "TOGGLE_LED")
        String type,

        @Schema(description = "Payload for the command", example = "{\"state\": \"ON\"}")
        Map<String, Object> payload
) {
}

