package com.example.iot.backend.service;

import com.example.iot.backend.mqtt.MqttPublisher;
import com.example.iot.backend.repository.DeviceRepository;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;

@Slf4j
@Service
@RequiredArgsConstructor
public class CommandService {

    private final DeviceRepository deviceRepository;
    private final MqttPublisher mqttPublisher;
    private final ObjectMapper objectMapper;

    public void sendCommand(Long deviceId, Object commandPayload) {
        var device = deviceRepository.findById(deviceId)
                .orElseThrow();
        var location = device.getLocation();
        var user = location.getUser();

        var topic = String.format("customer/%d/location/%d/device/%d/cmd", user.getId(), location.getId(), deviceId);

        try {
            String jsonPayload = objectMapper.writeValueAsString(commandPayload);
            log.info("Sending command to device {}: {}", topic, jsonPayload);
            mqttPublisher.publish(jsonPayload, topic);
        } catch (JsonProcessingException e) {
            log.error("Failed to serialize command payload", e);
            throw new RuntimeException("Failed to serialize command payload", e);
        }
    }
}
