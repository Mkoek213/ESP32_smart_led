package com.example.iot.backend.service;

import com.example.iot.backend.dto.command.CommandRequest;
import com.example.iot.backend.dto.device.DeviceClaimRequest;
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
import static com.example.iot.backend.model.Device.Status.OFFLINE;
import static com.example.iot.backend.model.Device.Status.ONLINE;
import static com.example.iot.backend.model.Device.Status.UNCLAIMED;

import lombok.extern.slf4j.Slf4j;

@Slf4j
@Service
@RequiredArgsConstructor
public class DeviceService {

    private static final String FACTORY_RESET_COMMAND = "FACTORY_RESET";

    private final DeviceRepository deviceRepository;
    private final LocationRepository locationRepository;
    private final CommandService commandService;
    private final com.example.iot.backend.repository.TelemetryRepository telemetryRepository;

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
    public DeviceResponse updateDevice(Long userId, Long deviceId, DeviceUpdateRequest request) {
        var device = getDeviceOrThrow(userId, deviceId);

        device.setName(request.name());

        return DeviceResponse.toDeviceResponse(deviceRepository.save(device));
    }

    @Transactional
    public void deleteDevice(Long userId, Long deviceId) {
        log.info("Attempting to delete device {} for user {}", deviceId, userId);
        try {
            var device = getDeviceOrThrow(userId, deviceId);

            // Delete telemetry directly
            log.info("Deleting telemetry records for device {}", deviceId);
            telemetryRepository.deleteAllByDeviceId(deviceId);

            deviceRepository.delete(device);
            deviceRepository.flush();
            log.info("Device deleted successfully");
        } catch (Exception e) {
            log.error("Error deleting device", e);
            throw e;
        }
    }

    @Transactional
    public void setDeviceStatus(Long userId, Long deviceId, Device.Status deviceStatus) {
        var device = getDeviceOrThrow(userId, deviceId);
        device.setStatus(deviceStatus);
        deviceRepository.save(device);
    }

    @Transactional
    public void setDeviceStatus(String macAddress, Device.Status deviceStatus) {
        deviceRepository.findByMacAddress(macAddress)
                .ifPresent(device -> {
                    device.setStatus(deviceStatus);
                    deviceRepository.save(device);
                });
    }

    @Transactional
    public void unbindDevice(Long userId, Long deviceId) {
        var device = getDeviceOrThrow(userId, deviceId);

        commandService.sendCommand(userId, deviceId, new CommandRequest(FACTORY_RESET_COMMAND, null));

        device.setLocation(null);
        device.setName(null);
        device.setStatus(UNCLAIMED);

        deviceRepository.save(device);
    }

    @Transactional
    public DeviceResponse claimDevice(Long userId, DeviceClaimRequest request) {
        var location = locationRepository.findByIdAndUserId(request.locationId(), userId)
                .orElseThrow(() -> resourceNotFound(Location.class));

        // Sanitize MAC address to ensure consistency
        String macAddress = request.macAddress().trim().toUpperCase();

        var device = deviceRepository.findByMacAddressIgnoreCase(macAddress)
                .orElseGet(() -> Device.builder()
                        .macAddress(macAddress) // Use sanitized MAC
                        .proofOfPossession(request.proofOfPossession())
                        .status(UNCLAIMED)
                        .hardwareId(java.util.UUID.randomUUID().toString()) // Generate a random hardware ID for new
                                                                            // devices
                        .build());

        if (!device.getProofOfPossession().equals(request.proofOfPossession())) {
            throw new IllegalArgumentException("Invalid verification code for this device.");
        }

        if (device.getStatus().equals(ONLINE)) {
            throw new IllegalStateException(
                    "Device is currently online and cannot be claimed. Perform a factory reset first.");
        }

        device.setLocation(location);
        device.setName(request.name());
        device.setStatus(OFFLINE);

        return DeviceResponse.toDeviceResponse(deviceRepository.save(device));
    }

    @Transactional
    public void controlDevice(Long userId, Long deviceId, CommandRequest command) {
        commandService.sendCommand(userId, deviceId, command);
    }

    private Device getDeviceOrThrow(Long userId, Long deviceId) {
        return deviceRepository.findByIdAndLocationUserId(deviceId, userId)
                .orElseThrow(() -> resourceNotFound(Device.class));
    }
}
