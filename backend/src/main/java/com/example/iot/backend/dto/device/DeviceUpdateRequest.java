package com.example.iot.backend.dto.device;

import jakarta.validation.constraints.NotBlank;

public record DeviceUpdateRequest(
        @NotBlank String name
) {
}
