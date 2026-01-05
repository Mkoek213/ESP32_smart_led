package com.example.iot.backend.service;


import com.example.iot.backend.dto.telemetry.TelemetryDto;
import com.example.iot.backend.dto.telemetry.TelemetryResponse;
import com.example.iot.backend.model.Device;
import com.example.iot.backend.model.Telemetry;
import com.example.iot.backend.repository.DeviceRepository;
import com.example.iot.backend.repository.TelemetryRepository;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.Instant;
import java.util.List;
import java.util.Map;

import static com.example.iot.backend.exception.ResourceNotFoundException.resourceNotFound;
import static java.util.stream.Collectors.groupingBy;

@Slf4j
@Service
@RequiredArgsConstructor
public class TelemetryService {

    private final DeviceRepository deviceRepository;
    private final TelemetryRepository telemetryRepository;

    @Transactional(readOnly = true)
    public Map<Long, List<TelemetryResponse>> getUserTelemetry(Long userId, Long start, Long end) {
        long[] range = resolveTimeRange(start, end);

        return telemetryRepository.findAllByDeviceLocationUserIdAndTimestampBetween(
                        userId, range[0], range[1]
                ).stream()
                .map(TelemetryResponse::toTelemetryResponse)
                .collect(groupingBy(TelemetryResponse::deviceId));
    }

    @Transactional(readOnly = true)
    public List<TelemetryResponse> getDeviceTelemetry(Long userId, Long deviceId, Long start, Long end) {
        if (!deviceRepository.existsByIdAndLocationUserId(deviceId, userId)) {
            throw resourceNotFound(Device.class);
        }

        long[] range = resolveTimeRange(start, end);

        return telemetryRepository.findAllByDeviceIdAndTimestampBetween(
                        deviceId, range[0], range[1]
                ).stream()
                .map(TelemetryResponse::toTelemetryResponse)
                .toList();
    }

    @Transactional
    public void saveTelemetry(TelemetryDto dto, Long deviceId) {
        var device = deviceRepository.getReferenceById(deviceId);

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

    private long[] resolveTimeRange(Long start, Long end) {
        long startTimestamp = (start == null) ? Instant.now().toEpochMilli() : start;
        long endTimestamp = (end == null) ? Instant.now().toEpochMilli() : end;
        return new long[]{startTimestamp, endTimestamp};
    }
}
