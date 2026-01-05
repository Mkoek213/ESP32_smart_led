package com.example.iot.backend.dto.auth;

import lombok.Builder;

@Builder
public record AuthenticationResponse(
        String token
) {
}
