package com.example.iot.backend.service;

import com.example.iot.backend.repository.DeviceRepository;
import com.example.iot.backend.repository.LocationRepository;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;

import java.security.Principal;

@Service
@RequiredArgsConstructor
public class AuthorizationService {

    private final DeviceRepository deviceRepository;
    private final LocationRepository locationRepository;

    public boolean canAccessDevice(Principal principal, Long deviceId) {
        final String userEmail = principal.getName();
        return deviceRepository.existsByIdAndLocation_User_Email(deviceId, userEmail);
    }

    public boolean canAccessLocation(Principal principal, Long locationId) {
        final String userEmail = principal.getName();
        return locationRepository.existsByIdAndUser_Email(locationId, userEmail);
    }
}
