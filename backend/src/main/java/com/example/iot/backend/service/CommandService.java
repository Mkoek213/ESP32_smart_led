package com.example.iot.backend.service;

import com.example.iot.backend.dto.command.CommandRequest;
import com.example.iot.backend.model.Device;
import com.example.iot.backend.mqtt.MqttPublisher;
import com.example.iot.backend.repository.DeviceRepository;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;

import static com.example.iot.backend.exception.ResourceNotFoundException.resourceNotFound;

@Slf4j
@Service
@RequiredArgsConstructor
public class CommandService {

    private final DeviceRepository deviceRepository;
    private final MqttPublisher mqttPublisher;
    private final ObjectMapper objectMapper;

    public void sendCommand(Long userId, Long deviceId, CommandRequest command) {
        var device = deviceRepository.findByIdAndLocationUserId(deviceId, userId)
                .orElseThrow(() -> resourceNotFound(Device.class));

        var locationId = device.getLocation().getId();

        var topic = String.format("customer/%d/location/%d/device/%d/cmd", userId, locationId, deviceId);

        try {
            String jsonPayload = objectMapper.writeValueAsString(command);
            log.debug("Publishing to topic [{}]: {}", topic, jsonPayload);
            mqttPublisher.publish(jsonPayload, topic);
        } catch (JsonProcessingException e) {
            log.error("Failed to serialize command payload for device {}", deviceId, e);
        }
    }
}
