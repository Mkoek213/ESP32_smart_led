package com.karolbystrek.mqttdatahub.service;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import com.karolbystrek.mqttdatahub.dto.SensorDataDto;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.messaging.handler.annotation.Header;
import org.springframework.messaging.handler.annotation.Payload;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;

import static org.springframework.integration.mqtt.support.MqttHeaders.RECEIVED_TOPIC;

@Slf4j
@Service
@RequiredArgsConstructor
public class MqttIngestionService {

    private final SensorDataService sensorDataService;
    private final ObjectMapper objectMapper = new ObjectMapper().registerModule(new JavaTimeModule());

    @Transactional
    public void handleSensorData(@Payload String payload, @Header(RECEIVED_TOPIC) String topic) {
        log.info("Received message on topic {}: {}", topic, payload);

        try {
            String[] topicParts = topic.split("/");
            if (topicParts.length != 4) {
                log.error("Invalid topic format: {}. Expected: {customer}/{location}/{device}/sensor-data", topic);
                return;
            }

            String customerName = topicParts[0];
            String locationName = topicParts[1];
            String deviceName = topicParts[2];

            List<SensorDataDto> sensorDataDtos = objectMapper.readValue(payload, new TypeReference<>() {
            });

            log.info("Received sensor readings: {}", sensorDataDtos);

            sensorDataDtos.forEach(dto -> sensorDataService.saveSensorDataFor(dto, customerName, locationName, deviceName));

        } catch (JsonProcessingException e) {
            log.error("Failed to parse JSON payload: {}", payload, e);
        }
    }
}
