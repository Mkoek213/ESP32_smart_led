package com.example.iot.backend.dto.device;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;

public record CreateDeviceRequest(
        @NotNull Long locationId,
        @NotBlank String name
) {
}
