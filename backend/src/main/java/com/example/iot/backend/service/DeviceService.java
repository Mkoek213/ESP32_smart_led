package com.example.iot.backend.service;

import com.example.iot.backend.dto.device.DeviceCreateRequest;
import com.example.iot.backend.dto.device.DeviceResponse;
import com.example.iot.backend.dto.device.DeviceUpdateRequest;
import com.example.iot.backend.model.Device;
import com.example.iot.backend.model.Location;
import com.example.iot.backend.repository.DeviceRepository;
import com.example.iot.backend.repository.LocationRepository;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;

import static com.example.iot.backend.exception.ResourceNotFoundException.resourceNotFound;

@Service
@RequiredArgsConstructor
public class DeviceService {

    private final DeviceRepository deviceRepository;
    private final LocationRepository locationRepository;

    @Transactional(readOnly = true)
    public DeviceResponse getDevice(Long userId, Long deviceId) {
        return DeviceResponse.toDeviceResponse(getDeviceOrThrow(userId, deviceId));
    }

    @Transactional(readOnly = true)
    public List<DeviceResponse> getAllDevices(Long userId) {
        return deviceRepository.findAllByLocationUserId(userId).stream()
                .map(DeviceResponse::toDeviceResponse)
                .toList();
    }

    @Transactional
    public DeviceResponse createDevice(Long userId, DeviceCreateRequest request) {
        var location = locationRepository.findByIdAndUserId(request.locationId(), userId)
                .orElseThrow(() -> resourceNotFound(Location.class));

        var device = Device.builder()
                .name(request.name())
                .location(location)
                .build();

        return DeviceResponse.toDeviceResponse(deviceRepository.save(device));
    }

    @Transactional
    public DeviceResponse updateDevice(Long userId, Long deviceId, DeviceUpdateRequest request) {
        var device = getDeviceOrThrow(userId, deviceId);

        device.setName(request.name());

        return DeviceResponse.toDeviceResponse(deviceRepository.save(device));
    }

    @Transactional
    public void deleteDevice(Long userId, Long deviceId) {
        var device = getDeviceOrThrow(userId, deviceId);
        deviceRepository.delete(device);
    }

    @Transactional
    public void setDeviceStatus(Long userId, Long deviceId, Device.Status deviceStatus) {
        var device = getDeviceOrThrow(userId, deviceId);
        device.setStatus(deviceStatus);
        deviceRepository.save(device);
    }

    private Device getDeviceOrThrow(Long userId, Long deviceId) {
        return deviceRepository.findByIdAndLocationUserId(deviceId, userId)
                .orElseThrow(() -> resourceNotFound(Device.class));
    }
}
