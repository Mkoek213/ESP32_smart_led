package com.karolbystrek.mqttdatahub.service;

import com.karolbystrek.mqttdatahub.dto.TelemetryDto;
import com.karolbystrek.mqttdatahub.dto.TelemetryResponse;
import com.karolbystrek.mqttdatahub.model.Telemetry;
import com.karolbystrek.mqttdatahub.repository.TelemetryRepository;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

@Slf4j
@Service
@RequiredArgsConstructor
public class TelemetryService {

    private final DeviceService deviceService;
    private final TelemetryRepository telemetryRepository;

    @Transactional
    public void save(TelemetryDto dto, Long userId, Long locationId, Long deviceId) {
        var device = deviceService.getDeviceBy(deviceId, locationId);
        var telemetry = Telemetry.createFrom(dto, device);

        telemetryRepository.save(telemetry);
        log.info("\nSuccessfully saved telemetry from device: {} (user: {}, location: {})\n", device, userId, locationId);
    }

    @Transactional(readOnly = true)
    public Map<Long, List<TelemetryResponse>> getTelemetryFor(Long userId, List<Long> deviceIds) {
        var authorizedDeviceIds = deviceIds.stream()
                .filter(deviceId -> deviceService.existsForUser(userId, deviceId))
                .toList();


        return telemetryRepository.findByDeviceIdIn(authorizedDeviceIds).stream()
                .map(TelemetryResponse::fromSensorData)
                .collect(Collectors.groupingBy(TelemetryResponse::deviceId));
    }
}
