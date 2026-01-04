package com.example.iot.backend.dto;

import lombok.Builder;

@Builder
public record AuthenticationResponse(
        String token
) {
}
