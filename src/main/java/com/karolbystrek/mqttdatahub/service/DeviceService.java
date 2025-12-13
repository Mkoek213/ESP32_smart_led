package com.karolbystrek.mqttdatahub.service;

import com.karolbystrek.mqttdatahub.model.Device;
import com.karolbystrek.mqttdatahub.model.Location;
import com.karolbystrek.mqttdatahub.repository.DeviceRepository;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import static java.time.LocalDateTime.now;

@Service
@RequiredArgsConstructor
public class DeviceService {

    private final DeviceRepository deviceRepository;

    public Device getBy(Long deviceId, Location location) {
        return deviceRepository.findByIdAndLocationId(deviceId, location.getId())
                .orElseThrow(() ->
                        new IllegalArgumentException(String.format("Device with id '%s' for location '%s' not found.", deviceId, location.getName()))
                );
    }

    public Boolean existsForCustomer(Long customerId, Long deviceId) {
        return deviceRepository.existsByIdAndLocationCustomerId(deviceId, customerId);
    }

    @Transactional
    public void updateDeviceStatus(Long customerId, Long locationId, Long deviceId, String status) {
        deviceRepository.findById(deviceId).ifPresent(device -> {
            if (device.getLocation().getId().equals(locationId) &&
                    device.getLocation().getCustomer().getId().equals(customerId)) {
                device.setStatus(status);
                device.setUpdatedAt(now());
                deviceRepository.save(device);
            }
        });
    }

    public Device getBy(Long deviceId) {
        return deviceRepository.findById(deviceId)
                .orElseThrow(() -> new IllegalArgumentException("Device not found: " + deviceId));
    }
}
