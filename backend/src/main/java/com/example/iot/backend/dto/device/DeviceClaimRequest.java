package com.example.iot.backend.dto.device;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;

public record DeviceClaimRequest(
        @NotBlank(message = "MAC address is required")
        String macAddress,

        @NotBlank(message = "Proof of possession is required")
        String proofOfPossession,

        @NotBlank(message = "Device name is required")
        String name,

        @NotNull(message = "Location ID is required")
        Long locationId
) {
}
