package com.example.iot.backend.dto.user;

public record UserUpdateRequest(
        String firstName,
        String lastName
) {
}
