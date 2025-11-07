package com.karolbystrek.mqttdatahub.service;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import com.karolbystrek.mqttdatahub.dto.SensorReadingDto;
import com.karolbystrek.mqttdatahub.model.Device;
import com.karolbystrek.mqttdatahub.model.SensorReading;
import com.karolbystrek.mqttdatahub.repository.DeviceRepository;
import com.karolbystrek.mqttdatahub.repository.SensorReadingRepository;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.messaging.handler.annotation.Header;
import org.springframework.messaging.handler.annotation.Payload;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.Optional;

import static java.lang.Long.getLong;
import static org.springframework.integration.mqtt.support.MqttHeaders.RECEIVED_TOPIC;

@Slf4j
@Service
@RequiredArgsConstructor
public class MqttIngestionService {

    private final SensorReadingRepository sensorReadingRepository;
    private final DeviceRepository deviceRepository;
    private final ObjectMapper objectMapper = new ObjectMapper().registerModule(new JavaTimeModule());

    @Transactional
    public void handleMessage(@Payload String payload, @Header(RECEIVED_TOPIC) String topic) {
        log.info("Received message on topic {}: {}", topic, payload);

        try {
            // Parse topic: {environment}/{customer}/{location}/{device}/sensor-data
            String[] topicParts = topic.split("/");
            if (topicParts.length != 5) {
                log.error("Invalid topic format: {}. Expected: {environment}/{customer_id}/{location_id}/{device_id}/sensor-data", topic);
                return;
            }

            Long customerId = getLong(topicParts[1]);
            Long locationId = getLong(topicParts[2]);
            Long deviceId = getLong(topicParts[3]);

            // Parse JSON payload
            SensorReadingDto dto = objectMapper.readValue(payload, SensorReadingDto.class);

            // Find or create entities
            Optional<Device> device = deviceRepository.findById(deviceId);

            SensorReading sensorReading = SensorReading.builder()
                    .humidity(dto.humidity())
                    .motionDetected(dto.motionDetected())
                    .lightLevel(dto.lightLevel())
                    .timestamp(dto.timestamp())
                    .device(device.orElse(null))
                    .build();

            sensorReadingRepository.save(sensorReading);
            log.info("Successfully saved sensor reading from device: {} (customer: {}, location: {})",
                    deviceId, customerId, locationId);

        } catch (JsonProcessingException e) {
            log.error("Failed to parse JSON payload: {}", payload, e);
        } catch (Exception e) {
            log.error("Error processing MQTT message from topic: {}", topic, e);
        }
    }
}
