package com.example.iot.backend.service;

import com.example.iot.backend.dto.device.DeviceCreateRequest;
import com.example.iot.backend.dto.device.DeviceResponse;
import com.example.iot.backend.model.Device;
import com.example.iot.backend.repository.DeviceRepository;
import com.example.iot.backend.repository.LocationRepository;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import static java.time.LocalDateTime.now;

@Service
@RequiredArgsConstructor
public class DeviceService {

    private final DeviceRepository deviceRepository;
    private final LocationRepository locationRepository;

    public DeviceResponse getDevice(Long deviceId) {
        var device = deviceRepository.findById(deviceId)
                .orElseThrow();
        return DeviceResponse.toDeviceResponse(device);
    }

    @Transactional
    public DeviceResponse createDevice(DeviceCreateRequest request) {
        var location = locationRepository.findById(request.locationId())
                .orElseThrow();
        var device = Device.builder()
                .name(request.name())
                .location(location)
                .build();

        deviceRepository.save(device);
        return DeviceResponse.toDeviceResponse(device);
    }

    @Transactional
    public void updateDeviceStatus(Long userId, Long locationId, Long deviceId, String status) {
        deviceRepository.findById(deviceId).ifPresent(device -> {
            if (device.getLocation().getId().equals(locationId) &&
                    device.getLocation().getUser().getId().equals(userId)) {
                device.setStatus(Device.Status.valueOf(status));
                device.setUpdatedAt(now());
                deviceRepository.save(device);
            }
        });
    }
}
