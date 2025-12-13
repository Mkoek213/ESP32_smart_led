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

    private final CustomerService customerService;
    private final LocationService locationService;
    private final DeviceService deviceService;
    private final TelemetryRepository telemetryRepository;

    @Transactional
    public void save(TelemetryDto dto, Long customerId, Long locationId, Long deviceId) {
        var customer = customerService.getBy(customerId);
        var location = locationService.getBy(locationId, customer);
        var device = deviceService.getBy(deviceId, location);

        var telemetry = Telemetry.createFrom(dto, device);

        telemetryRepository.save(telemetry);
        log.info("\nSuccessfully saved telemetry from device: {} (customer: {}, location: {})\n", device, customerId, locationId);
    }

    @Transactional(readOnly = true)
    public Map<Long, List<TelemetryResponse>> getTelemetryFor(Long customerId, List<Long> deviceIds) {
        var authorizedDeviceIds = deviceIds.stream()
                .filter(deviceId -> deviceService.existsForCustomer(customerId, deviceId))
                .toList();


        return telemetryRepository.findByDeviceIdIn(authorizedDeviceIds).stream()
                .map(TelemetryResponse::fromSensorData)
                .collect(Collectors.groupingBy(TelemetryResponse::deviceId));
    }
}
