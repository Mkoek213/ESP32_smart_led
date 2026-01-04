package com.karolbystrek.mqttdatahub.service;

import com.karolbystrek.mqttdatahub.model.Device;
import com.karolbystrek.mqttdatahub.repository.DeviceRepository;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import static java.time.LocalDateTime.now;

@Service
@RequiredArgsConstructor
public class DeviceService {

    private final DeviceRepository deviceRepository;

    public Device getDeviceBy(Long deviceId, Long locationId) {
        return deviceRepository.findByIdAndLocationId(deviceId, locationId)
                .orElseThrow(() ->
                        new IllegalArgumentException(String.format("Device with id '%s' for location with id '%s' not found.", deviceId, locationId))
                );
    }

    public Boolean existsForUser(Long userId, Long deviceId) {
        return deviceRepository.existsByIdAndLocationUserId(deviceId, userId);
    }

    @Transactional
    public void updateDeviceStatus(Long userId, Long locationId, Long deviceId, String status) {
        deviceRepository.findById(deviceId).ifPresent(device -> {
            if (device.getLocation().getId().equals(locationId) &&
                    device.getLocation().getUser().getId().equals(userId)) {
                device.setStatus(status);
                device.setUpdatedAt(now());
                deviceRepository.save(device);
            }
        });
    }

    public Device getDeviceBy(Long deviceId) {
        return deviceRepository.findById(deviceId)
                .orElseThrow(() -> new IllegalArgumentException("Device not found: " + deviceId));
    }
}
