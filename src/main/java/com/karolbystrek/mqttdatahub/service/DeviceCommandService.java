package com.karolbystrek.mqttdatahub.service;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.karolbystrek.mqttdatahub.integration.MqttGateway;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;

@Slf4j
@Service
@RequiredArgsConstructor
public class DeviceCommandService {

    private final MqttGateway mqttGateway;
    private final ObjectMapper objectMapper;

    public void sendCommand(Long userId, Long locationId, Long deviceId, Object commandPayload) {
        var topic = String.format("customer/%d/location/%d/device/%d/cmd", userId, locationId, deviceId);

        try {
            String payloadJson = objectMapper.writeValueAsString(commandPayload);
            log.info("Sending command to device {}: {}", topic, payloadJson);
            mqttGateway.sendToMqtt(payloadJson, topic);
        } catch (JsonProcessingException e) {
            log.error("Failed to serialize command payload", e);
            throw new RuntimeException("Failed to serialize command payload", e);
        }
    }
}
