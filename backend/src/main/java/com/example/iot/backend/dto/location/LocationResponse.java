package com.example.iot.backend.dto.location;

import com.example.iot.backend.model.Location;
import lombok.Builder;

import java.time.LocalDateTime;

@Builder
public record LocationResponse(
        Long id,
        String name,
        LocalDateTime updatedAt,
        LocalDateTime createdAt
) {

    public static LocationResponse toLocationResponse(Location location) {
        return LocationResponse.builder()
                .id(location.getId())
                .name(location.getName())
                .updatedAt(location.getUpdatedAt())
                .createdAt(location.getCreatedAt())
                .build();
    }
}
