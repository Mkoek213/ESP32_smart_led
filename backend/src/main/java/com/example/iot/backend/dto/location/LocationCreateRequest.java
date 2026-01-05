package com.example.iot.backend.dto.location;


import jakarta.validation.constraints.NotBlank;

public record LocationCreateRequest(
        @NotBlank String name
) {
}
