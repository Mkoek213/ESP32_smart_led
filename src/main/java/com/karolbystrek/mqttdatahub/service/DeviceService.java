package com.karolbystrek.mqttdatahub.service;

import com.karolbystrek.mqttdatahub.model.Device;
import com.karolbystrek.mqttdatahub.model.Location;
import com.karolbystrek.mqttdatahub.repository.DeviceRepository;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;

@Service
@RequiredArgsConstructor
public class DeviceService {

    private final DeviceRepository deviceRepository;

    public Device getDeviceBy(String deviceName, Location location) {
        return deviceRepository.findByNameAndLocationId(deviceName, location.getId())
                .orElseThrow(() ->
                        new IllegalArgumentException(String.format("Device with name '%s' for location '%s' not found.", deviceName, location.getName()))
                );
    }

    public Boolean existsForCustomer(Long customerId, Long deviceId) {
        return deviceRepository.existsByIdAndLocationCustomerId(deviceId, customerId);
    }
}
