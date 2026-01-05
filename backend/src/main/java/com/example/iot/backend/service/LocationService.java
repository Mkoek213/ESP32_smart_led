package com.example.iot.backend.service;

import com.example.iot.backend.dto.location.LocationCreateRequest;
import com.example.iot.backend.dto.location.LocationResponse;
import com.example.iot.backend.model.Location;
import com.example.iot.backend.repository.LocationRepository;
import com.example.iot.backend.repository.UserRepository;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;

import static com.example.iot.backend.exception.ResourceNotFoundException.throwResourceNotFound;

@RequiredArgsConstructor
@Service
public class LocationService {

    private final LocationRepository locationRepository;
    private final UserRepository userRepository;

    @Transactional(readOnly = true)
    public List<LocationResponse> getAllLocations(Long userId) {
        return locationRepository.findAllByUserId(userId).stream()
                .map(LocationResponse::toLocationResponse)
                .toList();
    }

    @Transactional(readOnly = true)
    public List<LocationResponse> searchLocations(Long userId, String name) {
        return locationRepository.findAllByUserIdAndNameContainingIgnoreCase(userId, name).stream()
                .map(LocationResponse::toLocationResponse)
                .toList();
    }

    @Transactional
    public LocationResponse createLocation(Long userId, LocationCreateRequest request) {
        var user = userRepository.getReferenceById(userId);

        var location = Location.builder()
                .name(request.name())
                .user(user)
                .build();

        return LocationResponse.toLocationResponse(locationRepository.save(location));
    }

    @Transactional
    public LocationResponse updateLocation(Long userId, Long locationId, LocationCreateRequest request) {
        var location = locationRepository.findByIdAndUserId(locationId, userId)
                .orElseThrow(() -> throwResourceNotFound(Location.class));

        location.setName(request.name());

        return LocationResponse.toLocationResponse(locationRepository.save(location));
    }

    @Transactional
    public void deleteLocation(Long userId, Long locationId) {
        var location = locationRepository.findByIdAndUserId(locationId, userId)
                .orElseThrow(() -> throwResourceNotFound(Location.class));

        locationRepository.delete(location);
    }
}
