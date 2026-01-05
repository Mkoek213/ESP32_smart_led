package com.example.iot.backend.service;


import com.example.iot.backend.dto.telemetry.TelemetryDto;
import com.example.iot.backend.dto.telemetry.TelemetryResponse;
import com.example.iot.backend.model.Telemetry;
import com.example.iot.backend.repository.DeviceRepository;
import com.example.iot.backend.repository.TelemetryRepository;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;
import java.util.Map;

import static java.util.stream.Collectors.groupingBy;

@Slf4j
@Service
@RequiredArgsConstructor
public class TelemetryService {

    private final DeviceRepository deviceRepository;
    private final TelemetryRepository telemetryRepository;

    @Transactional
    public void saveTelemetry(TelemetryDto dto, Long deviceId) {
        var device = deviceRepository.findById(deviceId)
                .orElseThrow();

        var telemetry = Telemetry.builder()
                .device(device)
                .timestamp(dto.timestamp())
                .temperature(dto.temperature())
                .humidity(dto.humidity())
                .pressure(dto.pressure())
                .personCount(dto.personCount())
                .build();
        telemetryRepository.save(telemetry);
    }

    @Transactional(readOnly = true)
    public Map<Long, List<TelemetryResponse>> getTelemetry(Long userId) {
        var telemetry = telemetryRepository.findAllByDevice_Location_User_Id(userId);
        return telemetry.stream()
                .map(TelemetryResponse::toTelemetryResponse)
                .collect(groupingBy(TelemetryResponse::deviceId));
    }
}
