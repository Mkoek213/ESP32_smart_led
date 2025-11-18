package com.karolbystrek.mqttdatahub.service;

import com.karolbystrek.mqttdatahub.dto.SensorDataDto;
import com.karolbystrek.mqttdatahub.dto.SensorDataResponse;
import com.karolbystrek.mqttdatahub.model.Customer;
import com.karolbystrek.mqttdatahub.model.Device;
import com.karolbystrek.mqttdatahub.model.Location;
import com.karolbystrek.mqttdatahub.model.SensorData;
import com.karolbystrek.mqttdatahub.repository.SensorDataRepository;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import static com.karolbystrek.mqttdatahub.model.SensorData.createSensorDataFrom;

@Slf4j
@Service
@RequiredArgsConstructor
public class SensorDataService {

    private final CustomerService customerService;
    private final LocationService locationService;
    private final DeviceService deviceService;
    private final SensorDataRepository sensorDataRepository;

    @Transactional
    public void saveSensorDataFor(SensorDataDto dto, Long customerId, Long locationId, Long deviceId) {
        Customer customer = customerService.getCustomerBy(customerId);
        Location location = locationService.getLocationBy(locationId, customer);
        Device device = deviceService.getDeviceBy(deviceId, location);

        var sensorData = createSensorDataFrom(dto, device);

        sensorDataRepository.save(sensorData);
        log.info("\nSuccessfully saved sensor reading from device: {} (customer: {}, location: {})\n", device, customerId, locationId);
    }

    @Transactional(readOnly = true)
    public Map<Long, List<SensorDataResponse>> getSensorDataGroupedByDeviceId(Long customerId, List<Long> deviceIds) {
        var authorizedDeviceIds = deviceIds.stream()
                .filter(deviceId -> deviceService.existsForCustomer(customerId, deviceId))
                .toList();


        return sensorDataRepository.findByDeviceIdIn(authorizedDeviceIds).stream()
                .map(SensorDataResponse::fromSensorData)
                .collect(Collectors.groupingBy(SensorDataResponse::deviceId));
    }
}
