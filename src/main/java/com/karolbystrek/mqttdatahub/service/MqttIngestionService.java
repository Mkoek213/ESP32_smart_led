package com.karolbystrek.mqttdatahub.service;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import com.karolbystrek.mqttdatahub.dto.SensorDataDto;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.messaging.handler.annotation.Header;
import org.springframework.messaging.handler.annotation.Payload;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.LocalDate;
import java.util.List;

import static java.time.LocalDateTime.now;
import static org.springframework.integration.mqtt.support.MqttHeaders.RECEIVED_TOPIC;

@Slf4j
@Service
@RequiredArgsConstructor
public class MqttIngestionService {

    private final SensorDataService sensorDataService;
    private final ObjectMapper objectMapper = new ObjectMapper().registerModule(new JavaTimeModule());

    @Transactional
    public void handleSensorData(@Payload String payload, @Header(RECEIVED_TOPIC) String topic) {
        log.info("\nReceived message on topic {}: {}\n", topic, payload);

        try {
            String[] topicParts = topic.split("/");
            if (topicParts.length != 7) {
                log.error("Invalid topic format: {}. Expected: /customer/{customer_id}/location/{location_id}/device/{device_id}/sensor-data", topic);
                return;
            }

            Long customerId = Long.parseLong(topicParts[1]);
            Long locationId = Long.parseLong(topicParts[3]);
            Long deviceId = Long.parseLong(topicParts[5]);

            List<SensorDataDto> sensorDataDtos = objectMapper.readValue(payload, new TypeReference<>() {
            });

            log.info("\nReceived sensor readings at {}:\n{}\n", now(), sensorDataDtos);

            sensorDataDtos.forEach(dto -> sensorDataService.saveSensorDataFor(dto, customerId, locationId, deviceId));

        } catch (JsonProcessingException e) {
            log.error("Failed to parse JSON payload: {}", payload, e);
        }
    }
}
